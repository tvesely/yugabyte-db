setup
{
  CREATE TABLE IF NOT EXISTS products (
    product_id integer,
    date_added timestamp,
    name text,
    price numeric,
    discounted_price numeric,
    date_expires timestamptz,
    CHECK (price > 0),
    CHECK (discounted_price > 0),
    CHECK (date_expires > date_added),
    CHECK (price > discounted_price),
    PRIMARY KEY((product_id) HASH, date_added)
  ) SPLIT INTO 1 TABLETS;

  INSERT INTO products VALUES (1,'2022-01-01 05:00:00', 'oats', 10, 1, '2022-12-31 23:59:59');
}

teardown
{
  DELETE FROM products;
}

session "s1"
step "s1_serializable_txn"    { BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE; }
step "s1_repeatable_read_txn" { BEGIN TRANSACTION ISOLATION LEVEL REPEATABLE READ; }
step "s1_read_committed_txn" { BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED; }
step "s1_update1"	      { UPDATE products SET price = 3 WHERE product_id = 1; }
step "s1_update2"         { UPDATE products SET discounted_price = 4 WHERE product_id = 1; }
step "s1_update3"         { UPDATE products SET name = 'pork' WHERE product_id = 1; }
step "s1_update4"         { UPDATE products SET product_id = 11 WHERE product_id = 1; }
step "s1_update5"         { UPDATE products SET date_added = '2022-06-01 01:00:00' WHERE product_id = 1; }
step "s1_update6"         { UPDATE products SET date_expires = '2022-05-01 01:00:00' WHERE product_id = 1; }
step "s1_insert_on_conflict1" {
    INSERT INTO products VALUES(1,'2022-01-01 05:00:00', 'fish', 11, 2, '2022-12-30 23:59:59')
    ON CONFLICT(product_id, date_added) DO UPDATE SET discounted_price = 5;
}
step "s1_insert_on_conflict2" {
    INSERT INTO products VALUES(1,'2022-01-01 05:00:00', 'fish', 11, 2, '2022-12-30 23:59:59')
    ON CONFLICT(product_id, date_added) DO UPDATE SET date_expires = '2022-05-01 02:00:00';
}
step "s1_insert_on_conflict3" {
    INSERT INTO products VALUES(1,'2022-01-01 05:00:00', 'fish', 11, 2, '2022-12-30 23:59:59')
    ON CONFLICT(product_id, date_added) DO UPDATE SET date_added = '2022-05-01 02:00:00';
}
step "s1_commit"	      { COMMIT; }
step "s1_select"   { SELECT * FROM products; }
step "s1_priority" {  SET yb_transaction_priority_lower_bound = .9; }


session "s2"
step "s2_serializable_txn"    { BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE; }
step "s2_repeatable_read_txn" { BEGIN TRANSACTION ISOLATION LEVEL REPEATABLE READ; }
step "s2_read_committed_txn" { BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED; }
step "s2_update1"	      { UPDATE products SET price = 2 WHERE product_id = 1; }
step "s2_update2"         { UPDATE products SET discounted_price = 5 WHERE product_id = 1; }
step "s2_update3"         { UPDATE products SET name = 'pork' WHERE product_id = 1; }
step "s2_update4"         { UPDATE products SET product_id = 12 WHERE product_id = 1; }
step "s2_update5"         { UPDATE products SET date_added = '2022-06-01 02:00:00' WHERE product_id = 1; }
step "s2_update6"         { UPDATE products SET date_expires = '2022-05-01 02:00:00' WHERE product_id = 1; }
step "s2_insert_on_conflict1" {
    INSERT INTO products VALUES(1,'2022-01-01 05:00:00', 'fish', 11, 2, '2022-12-30 23:59:59')
    ON CONFLICT(product_id, date_added) DO UPDATE SET discounted_price = 5;
}
step "s2_insert_on_conflict2" {
    INSERT INTO products VALUES(1,'2022-01-01 05:00:00', 'fish', 11, 2, '2022-12-30 23:59:59')
    ON CONFLICT(product_id, date_added) DO UPDATE SET date_expires = '2022-05-01 02:00:00';
}
step "s2_insert_on_conflict3" {
    INSERT INTO products VALUES(1,'2022-01-01 05:00:00', 'fish', 11, 2, '2022-12-30 23:59:59')
    ON CONFLICT(product_id, date_added) DO UPDATE SET date_added = '2022-05-01 02:00:00';
}
step "s2_commit"          { COMMIT; }
step "s2_select"   { SELECT * from products; }
step "s2_priority" { SET yb_transaction_priority_upper_bound= .1; }

# Concurrent updates with check constraints

# ===========================
#        SERIALIZABLE
# ===========================
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update1" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update1" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update1" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update2" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update2" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update2" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update2" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update5" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update6" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update5" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update6" "s2_update5" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update1" "s2_insert_on_conflict1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update5" "s2_insert_on_conflict2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_update6" "s2_insert_on_conflict3" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_insert_on_conflict1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_insert_on_conflict2" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_select" "s1_serializable_txn" "s2_serializable_txn" "s1_insert_on_conflict3" "s2_update6" "s1_commit" "s2_commit" "s1_select"

# ===========================
#      REPEATABLE READ
# ===========================

# Because we have automatic retry on the first statement of a transaction,
# UPDATE with no SELECT should either succeed, or detect constraint violations
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update5" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update6" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update5" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update6" "s2_update5" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_insert_on_conflict1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update5" "s2_insert_on_conflict2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_update6" "s2_insert_on_conflict3" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_insert_on_conflict1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_insert_on_conflict2" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s1_select" "s2_repeatable_read_txn" "s1_insert_on_conflict3" "s2_update6" "s1_commit" "s2_commit" "s1_select"

# SELECT before UPDATE should cause conflict
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update1" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update1" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update1" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update2" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update2" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update2" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update2" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update5" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update6" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update5" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update6" "s2_update5" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update1" "s2_insert_on_conflict1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update5" "s2_insert_on_conflict2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_update6" "s2_insert_on_conflict3" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_insert_on_conflict1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_insert_on_conflict2" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_repeatable_read_txn" "s2_repeatable_read_txn" "s2_select" "s1_insert_on_conflict3" "s2_update6" "s1_commit" "s2_commit" "s1_select"
# ===========================
#       READ COMMITTED
# ===========================

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update1" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update2" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update5" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update6" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update5" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_repeatable_read_txn" "s1_update6" "s2_update5" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_read_committed_txn" "s1_update1" "s2_insert_on_conflict1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_read_committed_txn" "s1_update5" "s2_insert_on_conflict2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_read_committed_txn" "s1_update6" "s2_insert_on_conflict3" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_read_committed_txn" "s1_insert_on_conflict1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_read_committed_txn" "s1_insert_on_conflict2" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s1_select" "s2_read_committed_txn" "s1_insert_on_conflict3" "s2_update6" "s1_commit" "s2_commit" "s1_select"

# SELECT before UPDATE should not make a difference in read committed
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update1" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update1" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update1" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update2" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update2" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update2" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update2" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update3" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update3" "s2_update4" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update5" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update6" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update5" "s2_update6" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update6" "s2_update5" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update1" "s2_insert_on_conflict1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update5" "s2_insert_on_conflict2" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_update6" "s2_insert_on_conflict3" "s1_commit" "s2_commit" "s1_select"

permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_insert_on_conflict1" "s2_update1" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_insert_on_conflict2" "s2_update5" "s1_commit" "s2_commit" "s1_select"
permutation "s1_priority" "s2_priority" "s1_read_committed_txn" "s2_read_committed_txn" "s2_select" "s1_insert_on_conflict3" "s2_update6" "s1_commit" "s2_commit" "s1_select"
