-- directory paths and dlsuffix are passed to us in environment variables
\getenv libdir PG_LIBDIR
\getenv dlsuffix PG_DLSUFFIX

\set regresslib :libdir '/regress' :dlsuffix

CREATE FUNCTION test_support_func(internal)
    RETURNS internal
    AS :'regresslib', 'test_support_func'
    LANGUAGE C STRICT;
