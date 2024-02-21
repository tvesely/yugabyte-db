--
-- SCHEMA
--

-- Create 2 schemas with table of the same name in each.
CREATE SCHEMA S1;
CREATE SCHEMA S2;

CREATE TABLE S1.TBL (a1 int PRIMARY KEY);
CREATE TABLE S2.TBL (a2 text PRIMARY KEY);

-- Insert values into the tables and verify both can be queried.
INSERT INTO S1.TBL VALUES (1);
INSERT INTO S2.TBL VALUES ('a');

SELECT * FROM S1.TBL;
SELECT * FROM S2.TBL;

-- Drop one table and verify the other still exists.
DROP TABLE S1.TBL;
SELECT * FROM S2.TBL;

DROP TABLE S2.TBL;

-- verify yb_db_admin role can manage schemas like a superuser
CREATE SCHEMA test_ns_schema_other;
CREATE ROLE test_regress_user1;
SET SESSION AUTHORIZATION yb_db_admin;
ALTER SCHEMA test_ns_schema_other RENAME TO test_ns_schema_other_new;
ALTER SCHEMA test_ns_schema_other_new OWNER TO test_regress_user1;
DROP SCHEMA test_ns_schema_other_new;
-- verify that the objects were dropped
SELECT COUNT(*) FROM pg_class WHERE relnamespace =
    (SELECT oid FROM pg_namespace WHERE nspname = 'test_ns_schema_other_new');
CREATE SCHEMA test_ns_schema_yb_db_admin;
ALTER SCHEMA test_ns_schema_yb_db_admin RENAME TO test_ns_schema_yb_db_admin_new;
ALTER SCHEMA test_ns_schema_yb_db_admin_new OWNER TO test_regress_user1;
DROP SCHEMA test_ns_schema_yb_db_admin_new;
-- verify that the objects were dropped
SELECT COUNT(*) FROM pg_class WHERE relnamespace =
    (SELECT oid FROM pg_namespace WHERE nspname = 'test_ns_schema_yb_db_admin_new');
