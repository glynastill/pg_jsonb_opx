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
COMMENT ON FUNCTION jsonb_delete(jsonb, text) IS 'delete key in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, text);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = text);
COMMENT ON OPERATOR - (jsonb, text) IS 'delete key from left operand';

--
CREATE OR REPLACE FUNCTION jsonb_delete(jsonb, text[]) 
RETURNS jsonb
	AS 'MODULE_PATHNAME', 'jsonb_delete_keys'
LANGUAGE C IMMUTABLE STRICT;
COMMENT ON FUNCTION jsonb_delete(jsonb, text[]) IS 'delete keys in second argument from first argument';

DROP OPERATOR IF EXISTS - (jsonb, text[]);
CREATE OPERATOR - ( PROCEDURE = jsonb_delete, LEFTARG = jsonb, RIGHTARG = text[]);
COMMENT ON OPERATOR - (jsonb, text[]) IS 'delete keys from left operand';

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
