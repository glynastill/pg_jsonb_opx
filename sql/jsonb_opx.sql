CREATE EXTENSION jsonb_opx;

-------------------------------------------------------------------------------
-- Tests for jsonb - text
-------------------------------------------------------------------------------

-- text deletion from array containers will only delete string types as only strings can be keys:
SELECT '[1, "1", "2", 2]'::jsonb - '2'::text;

-- simple text deletion from an object container
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - 'b'::text;
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - 'b '::text;
SELECT '{"a": 1, "b": 2, "c": {"b": 3}}'::jsonb - 'b'::text;
SELECT '{"a": 1, "b": 2, "c": {"b": [1,2,3]}}'::jsonb - 'b'::text;
SELECT '{"a": 1, "b": 2, "c":[1,2,3]}'::jsonb - 'b'::text;
SELECT '{"a": 1, "b": 2, "c":[1,2,3]}'::jsonb - 'c'::text;

-- simple text deletion from an object container should only match keys
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - '3'::text;

-- others
SELECT '["1", "2", true, false]'::jsonb - '2'::text;
SELECT '["1", "2", "2", "2"]'::jsonb - '2'::text;
SELECT '["a",2,{"a":1, "b":2}]'::jsonb - 'a'::text;
SELECT '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb - 'a'::text;
SELECT '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb - 'd'::text;
SELECT '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb - 'b'::text;

-------------------------------------------------------------------------------
-- Tests for jsonb - text[]
-------------------------------------------------------------------------------

-- text[] deletion from array containers will only delete string types as only strings can be keys:
SELECT '[1, "1", "2", 2]'::jsonb - array['1','2'];

-- simple text[] deletion from an object container
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - ARRAY['a','b'];
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - ARRAY['a ','b ',' c'];
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - ARRAY['a','b','c'];
SELECT '{"a": 1, "b": 2, "c": {"b": 3}}'::jsonb - ARRAY['a','b']; 
SELECT '{"a": 1, "b": 2, "c": {"b": [1,2,3]}}'::jsonb - ARRAY['a','b'];
SELECT '{"a": 1, "b": 2, "c":[1,2,3]}'::jsonb - ARRAY['a','b'];
SELECT '{"a": 1, "b": 2, "c":[1,2,3]}'::jsonb - ARRAY['a','c'];
SELECT '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb - ARRAY['b','d'];
SELECT '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb - ARRAY['b','a'];
SELECT '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb - ARRAY['a','d'];

-- simple text[] deletion from an object container should only match keys or nulls
SELECT '{"a": 1, "b": 2, "c": 3}'::jsonb - ARRAY['1',' 2'];
SELECT '["a",2,{"a":1, "b":2}]'::jsonb - '{a}'::text[];
SELECT '["1",2]'::jsonb - ARRAY[null];
SELECT '["1",null,2]'::jsonb - ARRAY[null];

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
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}}'::jsonb - '{"a": "test2", "c": {"a": false}, "b": 2.2}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test2", "b": 2.3, "c": {"a": true}, "d":false, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test", "b": 2.3, "c": {"a": true}, "d":false, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test2", "b": 2.2, "c": {"a": true}, "d":false, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test2", "b": 2.3, "c": {"a": false}, "d":false, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test2", "b": 2.3, "c": {"a": true}, "d":true, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test2", "b": 2.3, "c": {"a": true}, "d":false, "e":[1,"a",2]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test2", "b": 2.3, "c": {"a": true}, "d":false, "e":[1,2,"a"]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test", "b": 2.2, "c": {"a": true}, "d":false, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test", "b": 2.2, "c": {"a": false}, "d":false, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,3]}'::jsonb;
SELECT '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb - '{"a": "test", "b": 2.2, "c": {"a": false}, "d":true, "e":[1,2,"a"]}'::jsonb;

-- known issues !!!!
-- lookups of lhs values in rhs jsonb use findJsonbValueFromContainer which does not allow looking up non-scalar elements resulting in "invalid jsonb scalar type"
SELECT '["a",2,{"a":1, "b":2}]'::jsonb - '[[1]]'::jsonb;
SELECT '["a",2,{"a":1, "b":2}]'::jsonb - '[{"a":1}]'::jsonb;

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
SELECT '["a","b"]'::jsonb || '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb;
SELECT '{"a":{"b":3, "c":[1,2,3,4]}, "d":2}'::jsonb || '["a","b"]'::jsonb;

-------------------------------------------------------------------------------
-- Tests for jsonb =# jsonb
-------------------------------------------------------------------------------

-- any keys existing in left argument have values replaced with those from righ argument
SELECT '{"a": 1, "b": 2, "c":[1,2,3], "d":{"test":false}}'::jsonb #= '{"a": [1,2,3,4], "b": {"f":100, "j":{"k":200}}, "c": 4, "d":{"test":true}}'::jsonb;

-- note that as we are matching only keys and replacing values operation on an scalar/array elements effectively does nothing 
SELECT '{"a":[1,2], "b":2, "c":12}'::jsonb #= '["a","b","c"]'::jsonb;
SELECT '{"a":[1,2], "b":2, "c":12}'::jsonb #= '[1,2,3]'::jsonb;
SELECT '[1,2,3]'::jsonb #= '[1,2,3,4]'::jsonb;
SELECT '"a"'::jsonb #= '{"a":1, "b":2}'::jsonb;
SELECT '{"a":1, "b":2}'::jsonb #= '"a"'::jsonb;

-------------------------------------------------------------------------------
-- Tests for jsonb #- text[] 
-------------------------------------------------------------------------------
SELECT '"a"'::jsonb #- ARRAY['b'];
SELECT '["a"]'::jsonb #- ARRAY['b'];
SELECT '{"a":1}'::jsonb #- ARRAY['b'];

SELECT '"a"'::jsonb #- ARRAY['a'];
SELECT '["a"]'::jsonb #- ARRAY['a'];
SELECT '{"a":1}'::jsonb #- ARRAY['a'];

SELECT '["a", "b"]'::jsonb #- ARRAY['a'];
SELECT '{"a":1, "b":2}'::jsonb #- ARRAY['a'];
SELECT '{"a":{"b":1}, "c":2}'::jsonb #- ARRAY['a'];

SELECT '{"a":[1,2,3,4], "b":2}'::jsonb #- ARRAY['a'];
SELECT '{"a":[1,2,3,4], "b":2}'::jsonb #- ARRAY['b'];

SELECT '{"a":{"b":[1,2,3,["a","b"]]}, "c":2}'::jsonb #- ARRAY['c'];
SELECT '{"a":{"b":[1,2,3,["a","b"]]}, "c":2}'::jsonb #- ARRAY['a'];
SELECT '{"a":{"b":[1,2,3,["a","b"]]}, "c":2}'::jsonb #- ARRAY['a','c'];
SELECT '{"a":{"b":[1,2,3,["a","b"]]}, "c":2}'::jsonb #- ARRAY['a','b'];
SELECT '{"a":{"b":[1,2,3,["a","b"]]}, "c":2}'::jsonb #- ARRAY['a','b','c'];
SELECT '{"a":{"b":{"c":1}, "c":[1,2,3,["a","b"]]}, "d":3}'::jsonb #- ARRAY['a','b','c'];

SELECT '{"a":{"b":[1,2,3,["a","b"]], "c":[1,2,3,4]}, "d":2}'::jsonb #- ARRAY['a','b'];
SELECT '{"a":{"b":[1,2,3,["a","b"]], "c":[1,2,3,4]}, "d":2}'::jsonb #- ARRAY['a','c'];
SELECT '{"a":{"b":[1,2,3,["a","b"]], "c":[1,2,3,4]}, "d":2}'::jsonb #- ARRAY['a',null];

SELECT '{"a":{"b":[1,2,3,["a",{"b":3}]], "c":[1,2,3,4]}, "d":2}'::jsonb #- ARRAY['a','b'];

SELECT '{"a":{"b":3, "d":[1,{"Z":[1,[2,3]]}]}}'::jsonb #- ARRAY['a','d'];

SELECT '["a", {"b":[1,2,3,4,5]}, 1, "c"]'::jsonb #- ARRAY['a'];
SELECT '["a", {"b":[1,2,3,4,5]}, 1, "c"]'::jsonb #- ARRAY['c'];
SELECT '[1,[2,[3,[4,[5,6,7]]]],"a","b"]'::jsonb #- ARRAY['b'];
SELECT '[1,[2,[3,[4,[5,6,7]]]],"a","b"]'::jsonb #- ARRAY['a'];

-- expected limitation: cannot call with path deeper than 1 on a non-object
SELECT '["a", "b"]'::jsonb #- ARRAY['a','b'];

-------------------------------------------------------------------------------
-- Tests for jsonb_replace_path jsonb text[] 
-------------------------------------------------------------------------------
SELECT jsonb_replace_path('{"a":1, "b":2}', ARRAY['a'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1}}', ARRAY['a'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1}}', ARRAY['a','b'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1, "c":1}}', ARRAY['a','b'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1, "c":1}}', ARRAY['a','c'], '{"f":3}'::jsonb);

SELECT jsonb_replace_path('{"a":{"b":1, "c":2, "d":[1,2]}}', ARRAY['a','b'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1, "c":2, "d":[1,2]}}', ARRAY['a','c'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1, "c":2, "d":[1,2]}}', ARRAY['a','d'], '{"f":3}'::jsonb);

SELECT jsonb_replace_path('{"a":{"d":[1,{"Z":[1,[2,3]]}]}}', ARRAY['a','d'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1, "c":2, "d":[1,{"Z":[1,[2,3]]}]}}', ARRAY['a','b'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1, "c":2, "d":[1,{"Z":[1,[2,3]]}]}}', ARRAY['a','c'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":{"b":1, "c":2, "d":[1,{"Z":[1,[2,3]]}]}}', ARRAY['a','d'], '{"f":3}'::jsonb);
SELECT jsonb_replace_path('{"a":1, "b":null, "c":[1,2,null], "c":{"d":11}, "e":{"20":[100,"c"]}}', ARRAY['c'], '{"f":[[1],2], "g":"test", "h":{"i":{"j":null}}}');

SELECT jsonb_replace_path('"a"', ARRAY['a'], '{"f":10}'::jsonb); 
SELECT jsonb_replace_path('"a"', ARRAY['z'], '{"f":10}'::jsonb); 
SELECT jsonb_replace_path('[null, "a"]', ARRAY[null], '"b"'::jsonb); 
SELECT jsonb_replace_path('[1,2,3,"4"]', ARRAY['4'], '"5"'::jsonb); 
