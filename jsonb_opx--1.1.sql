\echo Use "CREATE EXTENSION jsonb_opx" to load this file. \quit

-- CREATE OR REPLACE FUNCTION jsonb_delete (jsonb, text) 
-- RETURNS jsonb
--     AS 'SELECT jsonb_delete($1, ARRAY[$2]);'
-- LANGUAGE SQL IMMUTABLE STRICT; 
-- COMMENT ON FUNCTION jsonb_delete(jsonb, text) IS 'delete key in second argument from first argument';

CREATE OR REPLACE FUNCTION jsonb_delete (jsonb, text) 
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_delete_key'
LANGUAGE C IMMUTABLE STRICT; 
COMMENT ON FUNCTION jsonb_delete(jsonb, text) IS 'delete key or string values in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, text);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = text);
COMMENT ON OPERATOR - (jsonb, text) IS 'delete key from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_delete(jsonb, numeric)
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_delete_key'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete(jsonb, numeric) IS 'delete numeric elements in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, numeric);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = numeric);
COMMENT ON OPERATOR - (jsonb, numeric) IS 'delete numeric elements from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_delete(jsonb, boolean)
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_delete_key'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete(jsonb, boolean) IS 'delete boolean elements in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, boolean);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = boolean);
COMMENT ON OPERATOR - (jsonb, boolean) IS 'delete boolean elements from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_delete(jsonb, text[]) 
RETURNS jsonb
	AS 'MODULE_PATHNAME', 'jsonb_delete_keys'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete(jsonb, text[]) IS 'delete keys or string values in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, text[]);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = text[]);
COMMENT ON OPERATOR - (jsonb, text[]) IS 'delete keys from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_delete(jsonb, numeric[])
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_delete_keys'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete(jsonb, numeric[]) IS 'delete numeric elements in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, numeric[]);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = numeric[]);
COMMENT ON OPERATOR - (jsonb, numeric[]) IS 'delete numeric elements from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_delete(jsonb, boolean[])
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_delete_keys'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete(jsonb, boolean[]) IS 'delete boolean elements in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, boolean[]);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = boolean[]);
COMMENT ON OPERATOR - (jsonb, boolean[]) IS 'delete boolean elements from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_delete(jsonb, jsonb) 
RETURNS jsonb
	AS 'MODULE_PATHNAME', 'jsonb_delete_jsonb'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete(jsonb, jsonb) IS 'delete matching pairs in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, jsonb);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = jsonb);
COMMENT ON OPERATOR - (jsonb, jsonb) IS 'delete matching pairs from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_concat(jsonb, jsonb)
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_concat_jsonb'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_concat(jsonb, jsonb) IS 'concatenate first and second jsonb arguments';

DROP OPERATOR IF EXISTS || (jsonb, jsonb);
CREATE OPERATOR || ( PROCEDURE = jsonb_concat, LEFTARG = jsonb, RIGHTARG = jsonb);
COMMENT ON OPERATOR || (jsonb, jsonb) IS 'concatenate jsonb types';

--

CREATE OR REPLACE FUNCTION jsonb_replace(jsonb, jsonb)
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_replace_jsonb'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_replace(jsonb, jsonb) IS 'replace occurrences of second jsonb argument in first';

DROP OPERATOR IF EXISTS #= (jsonb, jsonb);
CREATE OPERATOR #= ( PROCEDURE = jsonb_replace, LEFTARG = jsonb, RIGHTARG = jsonb);
COMMENT ON OPERATOR #= (jsonb, jsonb) IS 'replace values for matching keys in jsonb types';

-- 1.1 Extra functions start here

CREATE OR REPLACE FUNCTION jsonb_delete_path(jsonb, text[])
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_delete_path'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete_path(jsonb, text[]) IS 'follow path of keys in order supplied in array and delete end-point key value pair from jsonb';

DROP OPERATOR IF EXISTS #- (jsonb, text[]);
CREATE OPERATOR #- ( PROCEDURE = jsonb_delete_path, LEFTARG = jsonb, RIGHTARG = text[]);
COMMENT ON OPERATOR #- (jsonb, text[]) IS 'delete key path from left operand';

--

CREATE OR REPLACE FUNCTION jsonb_replace_path(jsonb, text[], jsonb)
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_replace_path'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_replace_path(jsonb, text[], jsonb) IS 'follow path of keys in order supplied in array and replace end-point key value pair with supplied jsonb';

--

CREATE OR REPLACE FUNCTION jsonb_append_path(jsonb, text[], jsonb)
RETURNS jsonb
    AS 'MODULE_PATHNAME', 'jsonb_append_path'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_append_path(jsonb, text[], jsonb) IS 'follow path of keys in order supplied in array and append to end-point key value pair with supplied jsonb';
