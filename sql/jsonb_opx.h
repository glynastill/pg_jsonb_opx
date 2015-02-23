#ifndef __JSONB_OPX_H__
#define __JSONB_OPX_H__

extern JsonbValue * pushJsonbBinary(JsonbParseState **pstate, JsonbContainer *jsonb_container);
extern JsonbValue * pushJsonbValueBlind(JsonbParseState **pstate, JsonbIteratorToken jsonb_iterator_token, JsonbValue *jsonb_iterator_value);
extern Jsonb * jsonbModifyPath(Jsonb *jsonb_a, ArrayType *array_path, Jsonb *jsonb_b);

#endif
