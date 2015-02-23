jsonb_opx
=========

Missing operators for jsonb in PostgreSQL 9.4, this may contain some errors and bad form as it's primarily just experimentation (i'm not a frequent C programmer; but everyone has to start somewhere right?).  Please test that it suits your requirements before using in any production scenario.

Provides
--------

The following behave like hstore 1.x operators, i.e. without nested jsonb traversal

* deletion using **-** operator
  * jsonb_delete(jsonb, text)
  * jsonb_delete(jsonb, text[])
  * jsonb_delete(jsonb, jsonb)
* concatenation using **||** operator
  * jsonb_concat(jsonb, jsonb)
* replacement using **=#** operator
  * jsonb_replace(jsonb, jsonb)

The following are intended to eventually function like hstore 2.0 operators

* deletion at chained path using **#-** operator
    jsonb_delete_path(jsonb, text[])
* replacement at chained path using function
    jsonb_replace_path(jsonb, text[], jsonb)
