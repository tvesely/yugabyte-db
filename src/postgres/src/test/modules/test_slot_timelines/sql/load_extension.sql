CREATE EXTENSION test_slot_timelines;

SELECT test_slot_timelines_create_logical_slot('test_slot', 'test_decoding');

SELECT test_slot_timelines_advance_logical_slot('test_slot', txid_current(), txid_current(), pg_current_xlog_location(), pg_current_xlog_location());

SELECT pg_drop_replication_slot('test_slot');
