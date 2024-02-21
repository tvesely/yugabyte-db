--
-- A collection of queries to build the tenk1 table.
--
-- The queries are taken from the relevant dependency files.  Since it is
-- faster to run this rather than each file itself (e.g. dependency chain
-- test_setup, create_index), prefer using this.
--

--
-- test_setup
--

\getenv abs_srcdir PG_ABS_SRCDIR

--

CREATE TABLE tenk1 (
	unique1		int4,
	unique2		int4,
	two			int4,
	four		int4,
	ten			int4,
	twenty		int4,
	hundred		int4,
	thousand	int4,
	twothousand	int4,
	fivethous	int4,
	tenthous	int4,
	odd			int4,
	even		int4,
	stringu1	name,
	stringu2	name,
	string4		name
);
\set filename :abs_srcdir '/data/tenk.data'
COPY tenk1 FROM :'filename';
VACUUM ANALYZE tenk1;

--
-- yb_pg_create_index
-- (With modification to make them all nonconcurrent for performance.)
--

CREATE INDEX NONCONCURRENTLY tenk1_unique1 ON tenk1 USING btree(unique1 int4_ops ASC);

CREATE INDEX NONCONCURRENTLY tenk1_unique2 ON tenk1 USING btree(unique2 int4_ops ASC);

CREATE INDEX NONCONCURRENTLY tenk1_hundred ON tenk1 USING btree(hundred int4_ops ASC);

CREATE INDEX NONCONCURRENTLY tenk1_thous_tenthous ON tenk1 (thousand ASC, tenthous ASC);
