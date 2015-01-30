jsonb_opx
=========

Test delete and concatenation operators for PostgreSQL 9.4, this is purely for experimentation and contain errors and bad form.  

**USE ON PRODUCTION SYSTEMS AT OWN RISK**

* delete operator **"-"** provided by functions *jsonb_delete(jsonb, text) jsonb_delete(jsonb, text[]) and jsonb_delete(jsonb, jsonb)*
    Provides:
        jsonb - text
        jsonb - text[]
        jsonb - jsonb

    Note: When using text type operators on jsonb arrays only elements of type text will be deleted. E.g.

```sql
TEST=# SELECT '[1, "1", "2", 2]'::jsonb - '2'::text;
  ?column?   
-------------
 [1, "1", 2]
(1 row)

TEST=# SELECT '[1, "1", "2", 2]'::jsonb - '"2"'::text;
     ?column?     
------------------
 [1, "1", "2", 2]
(1 row)

TEST=# SELECT '[1, "1", "2", 2]'::jsonb - array['2']::text[];
  ?column?   
-------------
 [1, "1", 2]
(1 row)

TEST=# SELECT '[1, "1", "2", 2]'::jsonb - array['"2"']::text[];
     ?column?     
------------------
 [1, "1", "2", 2]
(1 row)

```

    More. E.g.
 
```sql
TEST=# SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - 'b'::text;
     ?column?     
------------------
 {"a": 1, "c": 3}
(1 row)

TEST=# SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - ARRAY['a','b'];
 ?column? 
----------
 {"c": 3}
(1 row)

TEST=# SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - '{"a": 4, "b": 2}'::jsonb;
     ?column?     
------------------
 {"a": 1, "c": 3}
(1 row)

TEST=# SELECT '{"a": 1, "b": 2, "c": 3, "d": {"a": 4}}'::jsonb - '{"d": {"a": 4}, "b": 2}'::jsonb;
     ?column?     
------------------
 {"a": 1, "c": 3}
(1 row)

TEST=# SELECT '{"a": 4, "b": 2, "c": 3, "d": {"a": 4}}'::jsonb - '{"a": 4, "b": 2}'::jsonb;
        ?column?         
-------------------------
 {"c": 3, "d": {"a": 4}}
(1 row)
```
    
* concatenation operator  **"||"** provided by function *jsonb_concat(jsonb, jsonb)*
    Provides:
         jsonb || jsonb

    E.g.

```sql
TEST=#  SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb || '{"a": 4, "b": 2, "d": 4}'::jsonb;
             ?column?             
----------------------------------
 {"a": 4, "b": 2, "c": 3, "d": 4}
(1 row)

TEST=#  SELECT '["a", "b"]'::jsonb || '["c"]'::jsonb;
    ?column?     
-----------------
 ["a", "b", "c"]
(1 row)

TEST=#  SELECT '[1,2,3]'::jsonb || '[3,4,5]'::jsonb;
      ?column?      
--------------------
 [1, 2, 3, 3, 4, 5]
(1 row)

TEST=#  SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb || '[1,2,3]'::jsonb;
              ?column?               
-------------------------------------
 [{"a": 1, "b": 2, "c": 3}, 1, 2, 3]
(1 row)

TEST=#  SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb || '[1,2,3]'::jsonb || '"a"'::jsonb;
                 ?column?                 
------------------------------------------
 [{"a": 1, "b": 2, "c": 3}, 1, 2, 3, "a"]
(1 row)
```
