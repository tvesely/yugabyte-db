--
-- A collection of queries to build the tenk2 table.
--
-- The queries are taken from the relevant dependency files.  Since it is
-- faster to run this rather than each file itself (e.g. dependency chain
-- test_setup, create_index), prefer using this.
--
-- DEPENDENCY: this file must be run after tenk1 has been populated (by
-- yb_dep_tenk1).
--

--
-- test_setup
--

CREATE TABLE tenk2 AS SELECT * FROM tenk1;
VACUUM ANALYZE tenk2;

--
-- yb_pg_create_index
-- (With modification to make them all nonconcurrent for performance.)
--

CREATE INDEX NONCONCURRENTLY tenk2_unique1 ON tenk2 USING btree(unique1 int4_ops ASC);

CREATE INDEX NONCONCURRENTLY tenk2_unique2 ON tenk2 USING btree(unique2 int4_ops ASC);

CREATE INDEX NONCONCURRENTLY tenk2_hundred ON tenk2 USING btree(hundred int4_ops ASC);
