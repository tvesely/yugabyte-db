/*-------------------------------------------------------------------------
 *
 * extended_stats.c
 *	  POSTGRES extended statistics
 *
 * Generic code supporting statistic objects created via CREATE STATISTICS.
 *
 *
 * Portions Copyright (c) 1996-2017, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/statistics/extended_stats.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "catalog/indexing.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_statistic_ext.h"
#include "nodes/relation.h"
#include "statistics/extended_stats_internal.h"
#include "statistics/statistics.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"


/*
 * Used internally to refer to an individual pg_statistic_ext entry.
 */
typedef struct StatExtEntry
{
	Oid			statOid;	/* OID of pg_statistic_ext entry */
	Bitmapset  *columns;	/* attribute numbers covered by the statistics */
	List	   *types;		/* 'char' list of enabled statistic kinds */
} StatExtEntry;


static List *fetch_statentries_for_relation(Relation pg_statext, Oid relid);
static VacAttrStats **lookup_var_attr_stats(Relation rel, Bitmapset *attrs,
					  int natts, VacAttrStats **vacattrstats);
static void statext_store(Relation pg_stext, Oid relid,
			  MVNDistinct *ndistinct, MVDependencies *dependencies,
			  VacAttrStats **stats);


/*
 * Compute requested extended stats, using the rows sampled for the plain
 * (single-column) stats.
 *
 * This fetches a list of stats from pg_statistic_ext, computes the stats
 * and serializes them back into the catalog (as bytea values).
 */
void
BuildRelationExtStatistics(Relation onerel, double totalrows,
						   int numrows, HeapTuple *rows,
						   int natts, VacAttrStats **vacattrstats)
{
	Relation	pg_stext;
	ListCell   *lc;
	List	   *stats;

	pg_stext = heap_open(StatisticExtRelationId, RowExclusiveLock);
	stats = fetch_statentries_for_relation(pg_stext, RelationGetRelid(onerel));

	foreach(lc, stats)
	{
		StatExtEntry   *stat = (StatExtEntry *) lfirst(lc);
		MVNDistinct	   *ndistinct = NULL;
		MVDependencies *dependencies = NULL;
		VacAttrStats  **stats;
		ListCell	   *lc2;

		/* filter only the interesting vacattrstats records */
		stats = lookup_var_attr_stats(onerel, stat->columns,
									  natts, vacattrstats);

		/* check allowed number of dimensions */
		Assert(bms_num_members(stat->columns) >= 2 &&
			   bms_num_members(stat->columns) <= STATS_MAX_DIMENSIONS);

		/* compute statistic of each type */
		foreach(lc2, stat->types)
		{
			char	t = (char) lfirst_int(lc2);

			if (t == STATS_EXT_NDISTINCT)
				ndistinct = statext_ndistinct_build(totalrows, numrows, rows,
													stat->columns, stats);
			else if (t == STATS_EXT_DEPENDENCIES)
				dependencies = statext_dependencies_build(numrows, rows,
													   stat->columns, stats);
		}

		/* store the statistics in the catalog */
		statext_store(pg_stext, stat->statOid, ndistinct, dependencies, stats);
	}

	heap_close(pg_stext, RowExclusiveLock);
}

/*
 * statext_is_kind_built
 *		Is this stat kind built in the given pg_statistic_ext tuple?
 */
bool
statext_is_kind_built(HeapTuple htup, char type)
{
	AttrNumber  attnum;

	switch (type)
	{
		case STATS_EXT_NDISTINCT:
			attnum = Anum_pg_statistic_ext_standistinct;
			break;

		case STATS_EXT_DEPENDENCIES:
			attnum = Anum_pg_statistic_ext_stadependencies;
			break;

		default:
			elog(ERROR, "unexpected statistics type requested: %d", type);
	}

	return !heap_attisnull(htup, attnum);
}

/*
 * Return a list (of StatExtEntry) of statistics for the given relation.
 */
static List *
fetch_statentries_for_relation(Relation pg_statext, Oid relid)
{
	SysScanDesc scan;
	ScanKeyData skey;
	HeapTuple   htup;
	List       *result = NIL;

	/*
	 * Prepare to scan pg_statistic_ext for entries having indrelid = this
	 * rel.
	 */
	ScanKeyInit(&skey,
				Anum_pg_statistic_ext_starelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(relid));

	scan = systable_beginscan(pg_statext, StatisticExtRelidIndexId, true,
							  NULL, 1, &skey);

	while (HeapTupleIsValid(htup = systable_getnext(scan)))
	{
		StatExtEntry *entry;
		Datum		datum;
		bool		isnull;
		int			i;
		ArrayType  *arr;
		char	   *enabled;
		Form_pg_statistic_ext staForm;

		entry = palloc0(sizeof(StatExtEntry));
		entry->statOid = HeapTupleGetOid(htup);
		staForm = (Form_pg_statistic_ext) GETSTRUCT(htup);
		for (i = 0; i < staForm->stakeys.dim1; i++)
		{
			entry->columns = bms_add_member(entry->columns,
											staForm->stakeys.values[i]);
		}

		/* decode the staenabled char array into a list of chars */
		datum = SysCacheGetAttr(STATEXTOID, htup,
								Anum_pg_statistic_ext_staenabled, &isnull);
		Assert(!isnull);
		arr = DatumGetArrayTypeP(datum);
		if (ARR_NDIM(arr) != 1 ||
			ARR_HASNULL(arr) ||
			ARR_ELEMTYPE(arr) != CHAROID)
			elog(ERROR, "staenabled is not a 1-D char array");
		enabled = (char *) ARR_DATA_PTR(arr);
		for (i = 0; i < ARR_DIMS(arr)[0]; i++)
		{
			Assert((enabled[i] == STATS_EXT_NDISTINCT) ||
				   (enabled[i] == STATS_EXT_DEPENDENCIES));
			entry->types = lappend_int(entry->types, (int) enabled[i]);
		}

		result = lappend(result, entry);
	}

	systable_endscan(scan);

	return result;
}

/*
 * Using 'vacattrstats' of size 'natts' as input data, return a newly built
 * VacAttrStats array which includes only the items corresponding to attributes
 * indicated by 'attrs'.
 */
static VacAttrStats **
lookup_var_attr_stats(Relation rel, Bitmapset *attrs, int natts,
					  VacAttrStats **vacattrstats)
{
	int			i = 0;
	int			x = -1;
	VacAttrStats **stats;
	Bitmapset  *matched = NULL;

	stats = (VacAttrStats **)
		palloc(bms_num_members(attrs) * sizeof(VacAttrStats *));

	/* lookup VacAttrStats info for the requested columns (same attnum) */
	while ((x = bms_next_member(attrs, x)) >= 0)
	{
		int		j;

		stats[i] = NULL;
		for (j = 0; j < natts; j++)
		{
			if (x == vacattrstats[j]->tupattnum)
			{
				stats[i] = vacattrstats[j];
				break;
			}
		}

		if (!stats[i])
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("extended statistics could not be collected for column \"%s\" of relation %s.%s",
							NameStr(RelationGetDescr(rel)->attrs[x - 1]->attname),
							get_namespace_name(rel->rd_rel->relnamespace),
							RelationGetRelationName(rel)),
					 errhint("Consider ALTER TABLE \"%s\".\"%s\" ALTER \"%s\" SET STATISTICS -1",
							 get_namespace_name(rel->rd_rel->relnamespace),
							 RelationGetRelationName(rel),
							 NameStr(RelationGetDescr(rel)->attrs[x - 1]->attname))));

		/*
		 * Check that we found a non-dropped column and that the attnum
		 * matches.
		 */
		Assert(!stats[i]->attr->attisdropped);
		matched = bms_add_member(matched, stats[i]->tupattnum);

		i++;
	}
	if (bms_subset_compare(matched, attrs) != BMS_EQUAL)
		elog(ERROR, "could not find all attributes in attribute stats array");
	bms_free(matched);

	return stats;
}

/*
 * statext_store
 *	Serializes the statistics and stores them into the pg_statistic_ext tuple.
 */
static void
statext_store(Relation pg_stext, Oid statOid,
			  MVNDistinct *ndistinct, MVDependencies *dependencies,
			  VacAttrStats **stats)
{
	HeapTuple	stup,
				oldtup;
	Datum		values[Natts_pg_statistic_ext];
	bool		nulls[Natts_pg_statistic_ext];
	bool		replaces[Natts_pg_statistic_ext];

	memset(nulls, 1, Natts_pg_statistic_ext * sizeof(bool));
	memset(replaces, 0, Natts_pg_statistic_ext * sizeof(bool));
	memset(values, 0, Natts_pg_statistic_ext * sizeof(Datum));

	/*
	 * Construct a new pg_statistic_ext tuple, replacing the calculated stats.
	 */
	if (ndistinct != NULL)
	{
		bytea	   *data = statext_ndistinct_serialize(ndistinct);

		nulls[Anum_pg_statistic_ext_standistinct - 1] = (data == NULL);
		values[Anum_pg_statistic_ext_standistinct - 1] = PointerGetDatum(data);
	}

	if (dependencies != NULL)
	{
		bytea	   *data = statext_dependencies_serialize(dependencies);

		nulls[Anum_pg_statistic_ext_stadependencies - 1] = (data == NULL);
		values[Anum_pg_statistic_ext_stadependencies - 1] = PointerGetDatum(data);
	}

	/* always replace the value (either by bytea or NULL) */
	replaces[Anum_pg_statistic_ext_standistinct - 1] = true;
	replaces[Anum_pg_statistic_ext_stadependencies - 1] = true;

	/* there should already be a pg_statistic_ext tuple */
	oldtup = SearchSysCache1(STATEXTOID, ObjectIdGetDatum(statOid));
	if (!HeapTupleIsValid(oldtup))
		elog(ERROR, "cache lookup failed for extended statistics %u", statOid);

	/* replace it */
	stup = heap_modify_tuple(oldtup,
							 RelationGetDescr(pg_stext),
							 values,
							 nulls,
							 replaces);
	ReleaseSysCache(oldtup);
	CatalogTupleUpdate(pg_stext, &stup->t_self, stup);

	heap_freetuple(stup);
}

/* initialize multi-dimensional sort */
MultiSortSupport
multi_sort_init(int ndims)
{
	MultiSortSupport mss;

	Assert(ndims >= 2);

	mss = (MultiSortSupport) palloc0(offsetof(MultiSortSupportData, ssup)
									 +sizeof(SortSupportData) * ndims);

	mss->ndims = ndims;

	return mss;
}

/*
 * Prepare sort support info using the given sort operator
 * at the position 'sortdim'
 */
void
multi_sort_add_dimension(MultiSortSupport mss, int sortdim, Oid oper)
{
	SortSupport		ssup = &mss->ssup[sortdim];

	ssup->ssup_cxt = CurrentMemoryContext;
	ssup->ssup_collation = DEFAULT_COLLATION_OID;
	ssup->ssup_nulls_first = false;
	ssup->ssup_cxt = CurrentMemoryContext;

	PrepareSortSupportFromOrderingOp(oper, ssup);
}

/* compare all the dimensions in the selected order */
int
multi_sort_compare(const void *a, const void *b, void *arg)
{
	MultiSortSupport mss = (MultiSortSupport) arg;
	SortItem   *ia = (SortItem *) a;
	SortItem   *ib = (SortItem *) b;
	int			i;

	for (i = 0; i < mss->ndims; i++)
	{
		int			compare;

		compare = ApplySortComparator(ia->values[i], ia->isnull[i],
									  ib->values[i], ib->isnull[i],
									  &mss->ssup[i]);

		if (compare != 0)
			return compare;
	}

	/* equal by default */
	return 0;
}

/* compare selected dimension */
int
multi_sort_compare_dim(int dim, const SortItem *a, const SortItem *b,
					   MultiSortSupport mss)
{
	return ApplySortComparator(a->values[dim], a->isnull[dim],
							   b->values[dim], b->isnull[dim],
							   &mss->ssup[dim]);
}

int
multi_sort_compare_dims(int start, int end,
						const SortItem *a, const SortItem *b,
						MultiSortSupport mss)
{
	int			dim;

	for (dim = start; dim <= end; dim++)
	{
		int			r = ApplySortComparator(a->values[dim], a->isnull[dim],
											b->values[dim], b->isnull[dim],
											&mss->ssup[dim]);

		if (r != 0)
			return r;
	}

	return 0;
}

/*
 * has_stats_of_kind
 *	Check that the list contains statistic of a given kind
 */
bool
has_stats_of_kind(List *stats, char requiredkind)
{
	ListCell   *l;

	foreach(l, stats)
	{
		StatisticExtInfo *stat = (StatisticExtInfo *) lfirst(l);

		if (stat->kind == requiredkind)
			return true;
	}

	return false;
}

/*
 * choose_best_statistics
 *		Look for statistics with the specified 'requiredkind' which have keys
 *		that match at least two attnums.
 *
 * The current selection criteria is very simple - we choose the statistics
 * referencing the most attributes with the least keys.
 *
 * XXX if multiple statistics exists of the same size matching the same number
 * of keys, then the statistics which are chosen depend on the order that they
 * appear in the stats list. Perhaps this needs to be more definitive.
 */
StatisticExtInfo *
choose_best_statistics(List *stats, Bitmapset *attnums, char requiredkind)
{
	ListCell   *lc;
	StatisticExtInfo *best_match = NULL;
	int			best_num_matched = 2;	/* goal #1: maximize */
	int			best_match_keys = (STATS_MAX_DIMENSIONS + 1);	/* goal #2: minimize */

	foreach(lc, stats)
	{
		StatisticExtInfo *info = (StatisticExtInfo *) lfirst(lc);
		int			num_matched;
		int			numkeys;
		Bitmapset  *matched;

		/* skip statistics that are not the correct type */
		if (info->kind != requiredkind)
			continue;

		/* determine how many attributes of these stats can be matched to */
		matched = bms_intersect(attnums, info->keys);
		num_matched = bms_num_members(matched);
		bms_free(matched);

		/*
		 * save the actual number of keys in the stats so that we can choose
		 * the narrowest stats with the most matching keys.
		 */
		numkeys = bms_num_members(info->keys);

		/*
		 * Use these statistics when it increases the number of matched
		 * clauses or when it matches the same number of attributes but these
		 * stats have fewer keys than any previous match.
		 */
		if (num_matched > best_num_matched ||
			(num_matched == best_num_matched && numkeys < best_match_keys))
		{
			best_match = info;
			best_num_matched = num_matched;
			best_match_keys = numkeys;
		}
	}

	return best_match;
}
