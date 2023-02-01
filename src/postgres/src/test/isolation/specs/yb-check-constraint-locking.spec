setup
{
  CREATE TABLE products (
    product_no integer PRIMARY KEY,
    name text,
    price numeric,
    CHECK (price > 0),
    discounted_price numeric,
    CHECK (discounted_price > 0),
    CHECK (price > discounted_price)
  );

  INSERT INTO products VALUES (1, 'oats', 10, 1);
}

teardown
{
  DROP TABLE products;
}

session "s1"
setup             { BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED; }
step "s1a"	      { UPDATE PRODUCTS SET price = 3 WHERE product_no = 1; }
step "s1b"	      { COMMIT; }
step "s1_select"  { SELECT price, discounted_price FROM products; }
step "s1_noop"    { SELECT 1; }


session "s2"
step "s2s_start"  { BEGIN TRANSACTION ISOLATION LEVEL SERIALIZABLE; }
step "s2rr_start" { BEGIN TRANSACTION ISOLATION LEVEL REPEATABLE READ; }
step "s2rc_start" { BEGIN TRANSACTION ISOLATION LEVEL READ COMMITTED; }
step "s2a"        { UPDATE PRODUCTS SET discounted_price = 4 WHERE product_no = 1; }
step "s2b"        { COMMIT; }
step "s2_select"  { SELECT price, discounted_price from products; }

# This test is to ensure that the CHECK constraint is taken into account by concurrent transactions
permutation "s2s_start"  "s1a" "s2a" "s1_select" "s1b" "s2b" "s1_select"
permutation "s2rr_start" "s1a" "s2a" "s1_select" "s1b" "s2b" "s1_select"
permutation "s2rc_start" "s1a" "s2a" "s1_select" "s1b" "s2b" "s1_select"

# Serializable transactions leave read marks behind, so we can't run a select ahead of time in s2
permutation "s2s_start" "s1_noop" "s1a" "s2a" "s1_select" "s1b" "s2b" "s1_select"

permutation "s2rr_start" "s2_select" "s1a" "s2a" "s1_select" "s1b" "s2b" "s1_select"
permutation "s2rc_start" "s2_select" "s1a" "s2a" "s1_select" "s1b" "s2b" "s1_select"
