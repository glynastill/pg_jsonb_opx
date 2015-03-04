jsonb_opx
=========

Missing operators for jsonb in PostgreSQL 9.4, this may contain some errors and bad form as it's primarily just experimentation (i'm not a frequent C programmer; but everyone has to start somewhere right?).  Please test that it suits your requirements before using in any production scenario.

Provides
--------

The following behave like **hstore 1.x operators**; i.e. without nested jsonb traversal

* deletion using **-** operator
  * jsonb_delete(jsonb, text)
  * jsonb_delete(jsonb, numeric)
  * jsonb_delete(jsonb, boolean)
  * jsonb_delete(jsonb, text[])
  * jsonb_delete(jsonb, numeric[])
  * jsonb_delete(jsonb, boolean[])
  * jsonb_delete(jsonb, jsonb)
* concatenation using **||** operator
  * jsonb_concat(jsonb, jsonb)
* replacement using **#=** operator
  * jsonb_replace(jsonb, jsonb)

All of the above are provided with the standard extension and can be installed via CREATE EXTENSION E.g:

```sql
CREATE EXTENSION jsonb_opx;
``` 

The following are intended to behave like **hstore 2.0 operators**;  i.e. recurse into nested jsonb path.

> As of 26/02/2015 there appears to be an effort discussed on pgsql-hackers for this type of path manipulation named <a href="http://github.com/erthalion/jsonbx" target="_blank">jsonbx</a> that appears to be much further ahead than my effort below.

* deletion at chained path using **#-** operator
  * jsonb_delete_path(jsonb, text[])
* replacement at chained path using function (no operator)
  * jsonb_replace_path(jsonb, text[], jsonb)

To install this extra functionality specify version 1.1 when using CREATE EXTENSION E.g:

```sql
CREATE EXTENSION jsonb_opx VERSION '1.1';
```

Or if you have version 1.0 already installed, use ALTER EXTENSION E.g:

```sql
ALTER EXTENSION jsonb_opx UPDATE TO '1.1';
```
