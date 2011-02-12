/*-------------------------------------------------------------------------
 *
 * pg_collation.h
 *	  definition of the system "collation" relation (pg_collation)
 *	  along with the relation's initial contents.
 *
 *
 * Portions Copyright (c) 1996-2010, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL$
 *
 * NOTES
 *	  the genbki.pl script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_COLLATION_H
#define PG_COLLATION_H

#include "catalog/genbki.h"

/* ----------------
 *		pg_collation definition.  cpp turns this into
 *		typedef struct FormData_pg_collation
 * ----------------
 */
#define CollationRelationId  3456

CATALOG(pg_collation,3456)
{
	NameData	collname;		/* collation name */
	Oid			collnamespace;	/* OID of namespace containing this collation */
	Oid			collowner;
	int4		collencoding;	/* encoding that this collation applies to */
	NameData	collcollate;	/* LC_COLLATE setting */
	NameData	collctype;		/* LC_CTYPE setting */
} FormData_pg_collation;

/* ----------------
 *		Form_pg_collation corresponds to a pointer to a row with
 *		the format of pg_collation relation.
 * ----------------
 */
typedef FormData_pg_collation *Form_pg_collation;

/* ----------------
 *		compiler constants for pg_collation
 * ----------------
 */
#define Natts_pg_collation				6
#define Anum_pg_collation_collname		1
#define Anum_pg_collation_collnamespace	2
#define Anum_pg_collation_collowner		3
#define Anum_pg_collation_collencoding	4
#define Anum_pg_collation_collcollate	5
#define Anum_pg_collation_collctype		6

DATA(insert OID = 100 ( default PGNSP PGUID 0 "" "" ));
DESCR("placeholder for default collation");
#define DEFAULT_COLLATION_OID			100

#endif   /* PG_COLLATION_H */
