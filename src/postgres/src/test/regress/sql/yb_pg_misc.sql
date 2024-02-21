--
-- MISC
--

-- directory paths and dlsuffix are passed to us in environment variables
\getenv abs_srcdir PG_ABS_SRCDIR
\getenv abs_builddir PG_ABS_BUILDDIR
\getenv libdir PG_LIBDIR
\getenv dlsuffix PG_DLSUFFIX

\set regresslib :libdir '/regress' :dlsuffix

-- TODO requires 'INHERITS' which is not supported.
CREATE FUNCTION overpaid(emp)
   RETURNS bool
   AS :'regresslib'
   LANGUAGE C STRICT;

CREATE FUNCTION reverse_name(name)
   RETURNS name
   AS :'regresslib'
   LANGUAGE C STRICT;

SELECT two, stringu1, ten, string4
   INTO TABLE tmp
   FROM onek;

\set filename :abs_builddir '/results/onek.data'
COPY onek TO :'filename';

-- YB note: create YB table instead of temp table to exercise YB tables.
CREATE TABLE onek_copy (LIKE onek);

COPY onek_copy FROM :'filename';

CREATE TABLE hobbies_r (
	name		text,
	person 		text
);

CREATE TABLE equipment_r (
	name 		text,
	hobby		text
);

CREATE FUNCTION hobbies(person)
   RETURNS setof hobbies_r
   AS 'select * from hobbies_r where person = $1.name'
   LANGUAGE SQL;

CREATE FUNCTION hobby_construct(text, text)
   RETURNS hobbies_r
   AS 'select $1 as name, $2 as hobby'
   LANGUAGE SQL;

CREATE FUNCTION hobby_construct_named(name text, hobby text)
   RETURNS hobbies_r
   AS 'select name, hobby'
   LANGUAGE SQL;

CREATE FUNCTION hobbies_by_name(hobbies_r.name%TYPE)
   RETURNS hobbies_r.person%TYPE
   AS 'select person from hobbies_r where name = $1'
   LANGUAGE SQL;

CREATE FUNCTION equipment(hobbies_r)
   RETURNS setof equipment_r
   AS 'select * from equipment_r where hobby = $1.name'
   LANGUAGE SQL;

CREATE FUNCTION equipment_named(hobby hobbies_r)
   RETURNS setof equipment_r
   AS 'select * from equipment_r where equipment_r.hobby = equipment_named.hobby.name'
   LANGUAGE SQL;

CREATE FUNCTION equipment_named_ambiguous_1a(hobby hobbies_r)
   RETURNS setof equipment_r
   AS 'select * from equipment_r where hobby = equipment_named_ambiguous_1a.hobby.name'
   LANGUAGE SQL;

CREATE FUNCTION equipment_named_ambiguous_1b(hobby hobbies_r)
   RETURNS setof equipment_r
   AS 'select * from equipment_r where equipment_r.hobby = hobby.name'
   LANGUAGE SQL;

CREATE FUNCTION equipment_named_ambiguous_1c(hobby hobbies_r)
   RETURNS setof equipment_r
   AS 'select * from equipment_r where hobby = hobby.name'
   LANGUAGE SQL;

CREATE FUNCTION equipment_named_ambiguous_2a(hobby text)
   RETURNS setof equipment_r
   AS 'select * from equipment_r where hobby = equipment_named_ambiguous_2a.hobby'
   LANGUAGE SQL;

CREATE FUNCTION equipment_named_ambiguous_2b(hobby text)
   RETURNS setof equipment_r
   AS 'select * from equipment_r where equipment_r.hobby = hobby'
   LANGUAGE SQL;

-- YB note: mimic natural discard of temp tables for tables that have been
-- converted from temp tables in the upstream PG to YB tables here.
DROP TABLE onek_copy;
