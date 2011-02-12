/*
 * This test is for Linux/glibc systems and assumes that a full set of
 * locales is installed.  It must be run in a UTF-8 locale.
 */

SET client_encoding TO UTF8;


CREATE TABLE collate_test1 (
    a int,
    b text COLLATE "en_US.utf8" NOT NULL
);

\d collate_test1

CREATE TABLE collate_test_fail (
    a int,
    b text COLLATE "ja_JP.eucjp"
);

CREATE TABLE collate_test_fail (
    a int,
    b text COLLATE "foo"
);

CREATE TABLE collate_test_fail (
    a int COLLATE "en_US.utf8",
    b text
);

CREATE TABLE collate_test_like (
    LIKE collate_test1
);

\d collate_test_like

CREATE TABLE collate_test2 (
    a int,
    b text COLLATE "sv_SE.utf8"
);

CREATE TABLE collate_test3 (
    a int,
    b text COLLATE "C"
);

INSERT INTO collate_test1 VALUES (1, 'abc'), (2, 'äbc'), (3, 'bbc'), (4, 'ABC');
INSERT INTO collate_test2 SELECT * FROM collate_test1;
INSERT INTO collate_test3 SELECT * FROM collate_test1;

SELECT * FROM collate_test1 WHERE b >= 'bbc';
SELECT * FROM collate_test2 WHERE b >= 'bbc';
SELECT * FROM collate_test3 WHERE b >= 'bbc';
SELECT * FROM collate_test3 WHERE b >= 'BBC';

SELECT * FROM collate_test1 WHERE b COLLATE "C" >= 'bbc';
SELECT * FROM collate_test1 WHERE b >= 'bbc' COLLATE "C";
SELECT * FROM collate_test1 WHERE b COLLATE "C" >= 'bbc' COLLATE "C";
SELECT * FROM collate_test1 WHERE b COLLATE "C" >= 'bbc' COLLATE "en_US.utf8";
SELECT * FROM collate_test1 WHERE b COLLATE "C" >= 'bbc' COLLATE "en_US";


CREATE DOMAIN testdomain_sv AS text COLLATE "sv_SE.utf8";
CREATE DOMAIN testdomain_i AS int COLLATE "sv_SE.utf8"; -- fails
CREATE TABLE collate_test4 (
    a int,
    b testdomain_sv
);
INSERT INTO collate_test4 SELECT * FROM collate_test1;
SELECT a, b FROM collate_test4 ORDER BY b;

CREATE TABLE collate_test5 (
    a int,
    b testdomain_sv COLLATE "en_US.utf8"
);
INSERT INTO collate_test5 SELECT * FROM collate_test1;
SELECT a, b FROM collate_test5 ORDER BY b;


SELECT a, b FROM collate_test1 ORDER BY b;
SELECT a, b FROM collate_test2 ORDER BY b;
SELECT a, b FROM collate_test3 ORDER BY b;

SELECT a, b FROM collate_test1 ORDER BY b COLLATE "C";

-- star expansion
SELECT * FROM collate_test1 ORDER BY b;
SELECT * FROM collate_test2 ORDER BY b;
SELECT * FROM collate_test3 ORDER BY b;

-- constant expression folding
SELECT 'bbc' COLLATE "en_US.utf8" > 'äbc' COLLATE "en_US.utf8" AS "true";
SELECT 'bbc' COLLATE "sv_SE.utf8" > 'äbc' COLLATE "sv_SE.utf8" AS "false";

-- upper/lower

CREATE TABLE collate_test10 (
    a int,
    x text COLLATE "en_US.utf8",
    y text COLLATE "tr_TR.utf8"
);

INSERT INTO collate_test10 VALUES (1, 'hij', 'hij'), (2, 'HIJ', 'HIJ');

SELECT a, lower(x), lower(y), upper(x), upper(y), initcap(x), initcap(y) FROM collate_test10;
SELECT a, lower(x COLLATE "C"), lower(y COLLATE "C") FROM collate_test10;

SELECT a, x, y FROM collate_test10 ORDER BY lower(y), a;

-- LIKE/ILIKE

SELECT * FROM collate_test1 WHERE b LIKE 'abc';
SELECT * FROM collate_test1 WHERE b LIKE 'abc%';
SELECT * FROM collate_test1 WHERE b LIKE '%bc%';
SELECT * FROM collate_test1 WHERE b ILIKE 'abc';
SELECT * FROM collate_test1 WHERE b ILIKE 'abc%';
SELECT * FROM collate_test1 WHERE b ILIKE '%bc%';

SELECT 'Türkiye' COLLATE "en_US.utf8" ILIKE '%KI%' AS "true";
SELECT 'Türkiye' COLLATE "tr_TR.utf8" ILIKE '%KI%' AS "false";

-- The following actually exercises the selectivity estimation for ILIKE.
SELECT relname FROM pg_class WHERE relname ILIKE 'abc%';


-- to_char

SET lc_time TO 'tr_TR.utf8';
SELECT to_char(date '2010-04-01', 'DD TMMON YYYY');
SELECT to_char(date '2010-04-01', 'DD TMMON YYYY' COLLATE "tr_TR.utf8");


-- backwards parsing

CREATE VIEW collview1 AS SELECT * FROM collate_test1 WHERE b COLLATE "C" >= 'bbc';
CREATE VIEW collview2 AS SELECT a, b FROM collate_test1 ORDER BY b COLLATE "C";
CREATE VIEW collview3 AS SELECT a, lower((x || x) COLLATE "C") FROM collate_test10;

SELECT table_name, view_definition FROM information_schema.views
  WHERE table_name LIKE 'collview%' ORDER BY 1;


-- collation propagation in various expression type

SELECT a, coalesce(b, 'foo') FROM collate_test1 ORDER BY 2;
SELECT a, coalesce(b, 'foo') FROM collate_test2 ORDER BY 2;
SELECT a, coalesce(b, 'foo') FROM collate_test3 ORDER BY 2;
SELECT a, lower(coalesce(x, 'foo')), lower(coalesce(y, 'foo')) FROM collate_test10;

SELECT a, b, greatest(b, 'CCC') FROM collate_test1 ORDER BY 3;
SELECT a, b, greatest(b, 'CCC') FROM collate_test2 ORDER BY 3;
SELECT a, b, greatest(b, 'CCC') FROM collate_test3 ORDER BY 3;
SELECT a, x, y, lower(greatest(x, 'foo')), lower(greatest(y, 'foo')) FROM collate_test10;

SELECT a, nullif(b, 'abc') FROM collate_test1 ORDER BY 2;
SELECT a, nullif(b, 'abc') FROM collate_test2 ORDER BY 2;
SELECT a, nullif(b, 'abc') FROM collate_test3 ORDER BY 2;
SELECT a, lower(nullif(x, 'foo')), lower(nullif(y, 'foo')) FROM collate_test10;

SELECT a, CASE b WHEN 'abc' THEN 'abcd' ELSE b END FROM collate_test1 ORDER BY 2;
SELECT a, CASE b WHEN 'abc' THEN 'abcd' ELSE b END FROM collate_test2 ORDER BY 2;
SELECT a, CASE b WHEN 'abc' THEN 'abcd' ELSE b END FROM collate_test3 ORDER BY 2;

CREATE DOMAIN testdomain AS text;
SELECT a, b::testdomain FROM collate_test1 ORDER BY 2;
SELECT a, b::testdomain FROM collate_test2 ORDER BY 2;
SELECT a, b::testdomain FROM collate_test3 ORDER BY 2;
SELECT a, b::testdomain_sv FROM collate_test3 ORDER BY 2;
SELECT a, lower(x::testdomain), lower(y::testdomain) FROM collate_test10;

SELECT min(b), max(b) FROM collate_test1;
SELECT min(b), max(b) FROM collate_test2;
SELECT min(b), max(b) FROM collate_test3;

SELECT array_agg(b ORDER BY b) FROM collate_test1;
SELECT array_agg(b ORDER BY b) FROM collate_test2;
SELECT array_agg(b ORDER BY b) FROM collate_test3;

SELECT a, b FROM collate_test1 UNION ALL SELECT a, b FROM collate_test1 ORDER BY 2;
SELECT a, b FROM collate_test2 UNION SELECT a, b FROM collate_test2 ORDER BY 2;
SELECT a, b FROM collate_test3 WHERE a < 4 INTERSECT SELECT a, b FROM collate_test3 WHERE a > 1 ORDER BY 2;
SELECT a, b FROM collate_test3 EXCEPT SELECT a, b FROM collate_test3 WHERE a < 2 ORDER BY 2;

SELECT a, b FROM collate_test1 UNION ALL SELECT a, b FROM collate_test3 ORDER BY 2; -- fail
SELECT a, b FROM collate_test1 UNION ALL SELECT a, b FROM collate_test3; -- ok
SELECT a, b FROM collate_test1 UNION SELECT a, b FROM collate_test3 ORDER BY 2; -- fail
SELECT a, b COLLATE "C" FROM collate_test1 UNION SELECT a, b FROM collate_test3 ORDER BY 2; -- ok
SELECT a, b FROM collate_test1 INTERSECT SELECT a, b FROM collate_test3 ORDER BY 2; -- fail
SELECT a, b FROM collate_test1 EXCEPT SELECT a, b FROM collate_test3 ORDER BY 2; -- fail


-- casting

SELECT CAST('42' AS text COLLATE "C");

SELECT a, CAST(b AS varchar) FROM collate_test1 ORDER BY 2;
SELECT a, CAST(b AS varchar) FROM collate_test2 ORDER BY 2;
SELECT a, CAST(b AS varchar) FROM collate_test3 ORDER BY 2;


-- polymorphism

SELECT * FROM unnest((SELECT array_agg(b ORDER BY b) FROM collate_test1)) ORDER BY 1;
SELECT * FROM unnest((SELECT array_agg(b ORDER BY b) FROM collate_test2)) ORDER BY 1;
SELECT * FROM unnest((SELECT array_agg(b ORDER BY b) FROM collate_test3)) ORDER BY 1;

CREATE FUNCTION dup (f1 anyelement, f2 out anyelement, f3 out anyarray)
    AS 'select $1, array[$1,$1]' LANGUAGE sql;

SELECT a, (dup(b)).* FROM collate_test1 ORDER BY 2;
SELECT a, (dup(b)).* FROM collate_test2 ORDER BY 2;
SELECT a, (dup(b)).* FROM collate_test3 ORDER BY 2;


-- indexes

CREATE INDEX collate_test1_idx1 ON collate_test1 (b);
CREATE INDEX collate_test1_idx2 ON collate_test1 (b COLLATE "C");
CREATE INDEX collate_test1_idx3 ON collate_test1 ((b COLLATE "C")); -- this is different grammatically

CREATE INDEX collate_test1_idx4 ON collate_test1 (a COLLATE "C"); -- fail
CREATE INDEX collate_test1_idx5 ON collate_test1 ((a COLLATE "C")); -- fail

SELECT relname, pg_get_indexdef(oid) FROM pg_class WHERE relname LIKE 'collate_test%_idx%';


-- schema manipulation commands

CREATE ROLE regress_test_role;
CREATE SCHEMA test_schema;

CREATE COLLATION test0 (locale = 'en_US.utf8');
CREATE COLLATION test0 (locale = 'en_US.utf8'); -- fail
CREATE COLLATION test1 (lc_collate = 'en_US.utf8', lc_ctype = 'de_DE.utf8');
CREATE COLLATION test2 (locale = 'en_US'); -- fail
CREATE COLLATION test3 (lc_collate = 'en_US.utf8'); -- fail

CREATE COLLATION test4 FROM nonsense;
CREATE COLLATION test5 FROM test0;

SELECT collname, collencoding, collcollate, collctype FROM pg_collation WHERE collname LIKE 'test%' ORDER BY 1;

ALTER COLLATION test1 RENAME TO test11;
ALTER COLLATION test0 RENAME TO test11; -- fail
ALTER COLLATION test1 RENAME TO test22; -- fail

ALTER COLLATION test11 OWNER TO regress_test_role;
ALTER COLLATION test11 OWNER TO nonsense;
ALTER COLLATION test11 SET SCHEMA test_schema;

COMMENT ON COLLATION test0 IS 'US English';

SELECT collname, nspname, obj_description(pg_collation.oid, 'pg_collation')
    FROM pg_collation JOIN pg_namespace ON (collnamespace = pg_namespace.oid)
    WHERE collname LIKE 'test%'
    ORDER BY 1;

DROP COLLATION test0, test_schema.test11, test5;
DROP COLLATION test0; -- fail
DROP COLLATION IF EXISTS test0;

SELECT collname FROM pg_collation WHERE collname LIKE 'test%';

DROP SCHEMA test_schema;
DROP ROLE regress_test_role;


-- dependencies

CREATE COLLATION test0 (locale = 'en_US.utf8');

CREATE TABLE collate_dep_test1 (a int, b text COLLATE test0);
CREATE DOMAIN collate_dep_dom1 AS text COLLATE test0;
CREATE TYPE collate_dep_test2 AS (x int, y text COLLATE test0);
CREATE VIEW collate_dep_test3 AS SELECT text 'foo' COLLATE test0 AS foo;
CREATE TABLE collate_dep_test4t (a int, b text);
CREATE INDEX collate_dep_test4i ON collate_dep_test4t (b COLLATE test0);

DROP COLLATION test0 RESTRICT; -- fail
DROP COLLATION test0 CASCADE;

\d collate_dep_test1
\d collate_dep_test2

DROP TABLE collate_dep_test1, collate_dep_test4t;
DROP TYPE collate_dep_test2;
