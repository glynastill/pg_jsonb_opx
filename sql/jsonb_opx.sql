\i /usr/local/pgsql/share/contrib/jsonb_opx.sql

-------------------------------------------------------------------------------
-- Tests for jsonb - text
-------------------------------------------------------------------------------

-- text deletion from array containers will only delete string types as only strings can be keys:
SELECT '[1, "1", "2", 2]'::jsonb - '2'::text;

-- simple text deletion from an object container
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - 'b'::text;

-- simple text deletion from an object container should only match keys
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - '3'::text;

-- others
SELECT '["1", "2", true, false]'::jsonb - '2'::text;
SELECT '["1", "2", "2", "2"]'::jsonb - '2'::text;

-------------------------------------------------------------------------------
-- Tests for jsonb - text[]
-------------------------------------------------------------------------------

-- text[] deletion from array containers will only delete string types as only strings can be keys:
SELECT '[1, "1", "2", 2]'::jsonb - array['1','2'];

-- simple text[] deletion from an object container
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - ARRAY['a',' b'];

-- simple text[] deletion from an object container should only match keys
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - ARRAY['1',' 2'];

-------------------------------------------------------------------------------
-- Tests for jsonb - jsonb
-------------------------------------------------------------------------------

-- jsonb deletion from an object should match on key/value
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - '{"a": 4, "b": 2}'::jsonb;

-- jsonb deletion from an array should only match on key
SELECT '["a", "b", "c"]'::jsonb - '{"a": 4, "b": 2}'::jsonb;

-- jsonb deletion from nested objects should not be part matched
SELECT '{"a": 4, "b": 2, "c": 3, "d": {"a": 4}}'::jsonb - '{"a": 4, "b": 2}'::jsonb;

-- but a match of all nested values should narcg
SELECT '{"a": 4, "b": 2, "c": 3, "d": {"a": 4}}'::jsonb - '{"d": {"a": 4}, "b": 2}'::jsonb;

-- others
SELECT '{"a": 4, "b": 2, "c": 3, "d": {"a": false}}'::jsonb - '{"d": {"a": false}, "b": 2}'::jsonb;

-------------------------------------------------------------------------------
-- Tests for jsonb || jsonb
-------------------------------------------------------------------------------

-- duplicates should automatically be removed by lower level logic
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb || '{"a": 4, "b": 2, "d": 4}'::jsonb;

-- concatentation of arrays
SELECT '["a", "b"]'::jsonb || '["c"]'::jsonb;

-- concatentation of scalars and arrays should be wrapped into arrays
SELECT '["a", "b"]'::jsonb || '"c"'::jsonb;

-- likewise concatentation of objects and arrays should be wrapped into arrays
SELECT '["a", "b"]'::jsonb || '{"a": 4, "b": 2}'::jsonb;

-- and all concatentation should be in natural order supplied
SELECT '{"a": 4, "b": 2}'::jsonb || '["a", "b"]'::jsonb || '["c", "d"]'::jsonb;

-- others
SELECT 'false'::jsonb || '["a", "b"]'::jsonb || '["c", "d"]'::jsonb;
