---
title: PostgreSQL compatibility
linkTitle: PostgreSQL compatibility
description: Summary of YugabyteDB's PostgreSQL compatibility
image: /images/section_icons/secure/create-roles.png
menu:
  stable:
    identifier: explore-ysql-postgresql-compatibility
    parent: explore-ysql-language-features
    weight: 70
type: docs
---

YugabyteDB is a [PostgreSQL-compatible](https://www.yugabyte.com/tech/postgres-compatibility/) distributed database that supports the majority of PostgreSQL syntax. This means that existing applications built on PostgreSQL can often be migrated to YugabyteDB without changing application code.

Because YugabyteDB is PostgreSQL compatible, it works with the majority of PostgreSQL database tools such as various language drivers, ORM tools, schema migration tools, and many more third-party database tools.

Because YugabyteDB is a distributed database, supporting all PostgreSQL features easily in a distributed system is not always feasible. This page documents the known list of differences between PostgreSQL and YugabyteDB. You need to consider these differences while porting an existing application to YugabyteDB.

## Unsupported PostgreSQL features

The following PostgreSQL features are not supported in YugabyteDB:

- Pessimistic locking (except Read Committed, which is in beta, _supports_ [pessimistic locking](../../../architecture/transactions/read-committed/#cross-feature-interaction))
- Table locks
- [Inheritance](https://www.postgresql.org/docs/11/tutorial-inheritance.html)
- Exclusion Constraints
- GiST indexes
- Kerberos/GSSAPI
- Events (Listen/Notify)
- Drop primary key
- XML Functions
- XA syntax