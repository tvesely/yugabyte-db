SET LOCAL yb_non_ddl_txn_for_sys_tables_allowed TO true;

INSERT INTO pg_catalog.pg_proc (
  oid, proname, pronamespace, proowner, prolang, procost, prorows, provariadic, protransform,
  prokind, prosecdef, proleakproof, proisstrict, proretset, provolatile, proparallel, pronargs,
  pronargdefaults, prorettype, proargtypes, proallargtypes, proargmodes, proargnames,
  proargdefaults, protrftypes, prosrc, probin, proconfig, proacl
) VALUES
  (8058, 'yb_lock_status', 11, 10, 12, 1, 1000, 0, '-', 'f', false, false, true, true,
   'v', 's', 2, 0, 2249, '25 25', '{25,25,25,26,26,23,25,16,16,1114,1114,3802}', '{i,i,o,o,o,o,o,o,o,o,o,o}',
   '{tablet_id,transaction_id,locktype,database,relation,pid,mode,granted,fastpath,waitstart,waitend,ybdetails}',
   NULL, NULL, 'yb_lock_status', NULL, NULL, NULL)
ON CONFLICT DO NOTHING;

-- Create dependency records for everything we (possibly) created.
-- Since pg_depend has no OID or unique constraint, using PL/pgSQL instead.
DO $$
BEGIN
  IF NOT EXISTS (
    SELECT FROM pg_catalog.pg_depend
      WHERE refclassid = 1255 AND refobjid = 8058
  ) THEN
    INSERT INTO pg_catalog.pg_depend (
      classid, objid, objsubid, refclassid, refobjid, refobjsubid, deptype
    ) VALUES
      (0, 0, 0, 1255, 8058, 0, 'p');
  END IF;
END $$;
