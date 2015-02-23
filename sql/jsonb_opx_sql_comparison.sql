-- The functions in this script are SQL versions of the C ones for comparison
-- of performance between the two.

CREATE OR REPLACE FUNCTION jsonb_delete_left(a jsonb, b text)
RETURNS jsonb AS
$BODY$
SELECT COALESCE(
(
SELECT ('{' || string_agg(to_json(key) || ':' || value, ',') || '}')
FROM jsonb_each(a)
WHERE key <> b
)
, '{}')::jsonb;
$BODY$
LANGUAGE sql IMMUTABLE STRICT;

--

CREATE OR REPLACE FUNCTION jsonb_delete_left(a jsonb, b text[])
RETURNS jsonb AS
$BODY$
SELECT COALESCE(
(
SELECT ('{' || string_agg(to_json(key) || ':' || value, ',') || '}')
FROM jsonb_each(a)
WHERE key <> ALL(b)
)
, '{}')::jsonb;
$BODY$
LANGUAGE sql IMMUTABLE STRICT;

--

CREATE OR REPLACE FUNCTION jsonb_delete_left(a jsonb, b jsonb)
RETURNS jsonb AS
$BODY$
SELECT COALESCE(
(
SELECT ('{' || string_agg(to_json(key) || ':' || value, ',') || '}')
FROM jsonb_each(a)
WHERE NOT ('{' || to_json(key) || ':' || value || '}')::jsonb <@ b
)
, '{}')::jsonb;
$BODY$
LANGUAGE sql IMMUTABLE STRICT;

--

CREATE OR REPLACE FUNCTION jsonb_concat_left (a jsonb, b jsonb) 
RETURNS jsonb AS
$BODY$
SELECT json_object_agg(key, value)::jsonb FROM
(
  SELECT * FROM jsonb_each(a)
  UNION ALL
  SELECT * FROM jsonb_each(b)
) a;
$BODY$
LANGUAGE sql IMMUTABLE STRICT; 
