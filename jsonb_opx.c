/* 
 * jsonb_opx.c 
 *     Test jsonb delete and concatenate operator functions for 9.4
 *
 * Portions Copyright (c) 1996-2015, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 * Author: Glyn Astill <glyn@8kb.co.uk>
 *
 * This is purely experimentation and will contain many errors and bad form
 * DO NOT USE ON PRODUCTION SYSTEMS.
 *
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"
#include "utils/numeric.h"
#include "utils/jsonb.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "jsonb_opx.h"
#include "utils/lsyscache.h"

#ifdef PG_MODULE_MAGIC
	PG_MODULE_MAGIC;
#endif 

Datum jsonb_delete_key(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_delete_key);

/*
 * Operator function to delete key from left operand where a match is found in
 * the right operand.
 *
 * jsonb, text -> jsonb
 *
 */
Datum
jsonb_delete_key(PG_FUNCTION_ARGS)
{
    /* pointers to incoming jsonb and text data */
    Jsonb                  *input_jsonb = PG_GETARG_JSONB(0);
    Oid                     input_element_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
    text                   *input_text = NULL;
    Numeric                 input_numeric = NULL;
    bool                    input_bool = false;

    /* pointers to return jsonb value data and state to be converted to jsonb on return */
    JsonbParseState        *state = NULL;
    JsonbValue             *return_jsonb_value = NULL;

    /* pointer to iterator for input_jsonb value data */
    JsonbIterator          *jsonb_iterator;
    JsonbValue              jsonb_iterator_value;
    int32                   jsonb_iterator_token;

    /* variables used for skip logic */
    int32                   skip_level = 0;
    int32                   nest_level = 0;
    int32                   array_level = 0;
    bool                    push = true;

    /* make function polymorphic for text/numeric/boolean */ 
    if (!OidIsValid(input_element_type))
        elog(ERROR, "could not determine data type of input");
        
    if (input_element_type == TEXTOID)
        input_text = PG_GETARG_TEXT_P(1);
    else if (input_element_type == NUMERICOID)
        input_numeric = PG_GETARG_NUMERIC(1);
    else if (input_element_type == BOOLOID)
        input_bool = PG_GETARG_BOOL(1);
    else
        elog(ERROR, "invalid data type input: %i", input_element_type);

    /*
    * If we've been supplied with an existing key iterate round json data and rebuild 
    * with key/element excluded.
    *
    * skip_key, nest_level and array_level are crude counts to check if the the value 
    * for the key is closed and ensure we don't match on keys within nested objects.  
    * Because we are recursing into nested elements but blindly just pushing them onto 
    * the return value we can get away without deeper knowledge of the json?
    */

    jsonb_iterator = JsonbIteratorInit(&input_jsonb->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, false)) != WJB_DONE) 
    {
        push = true;
        switch (jsonb_iterator_token)
        {
            case WJB_BEGIN_ARRAY:
                array_level++;
                break;
            case WJB_BEGIN_OBJECT:
                nest_level++;
                break;
            case WJB_ELEM:
                if (skip_level == 0 && nest_level == 0 && array_level == 1) 
                {
                    if ((jsonb_iterator_value.type == jbvNumeric) && (input_element_type == NUMERICOID)) 
                    {
                        if (DatumGetBool(DirectFunctionCall2(numeric_eq,
                            PointerGetDatum(jsonb_iterator_value.val.numeric),
                                PointerGetDatum(input_numeric))))
                             push = false;
                    }
                    else if ((jsonb_iterator_value.type == jbvBool) && (input_element_type == BOOLOID))
                    {
                        if (jsonb_iterator_value.val.boolean == input_bool)
                            push = false;
                    }
                    /* sql null <> jsonb null */
                }
            case WJB_KEY:
                if (skip_level == 0 && ((jsonb_iterator_token == WJB_KEY && nest_level == 1 && array_level == 0) ||
                        (jsonb_iterator_token == WJB_ELEM && nest_level == 0 && array_level == 1)))
                {
                    if ((jsonb_iterator_value.type == jbvString) && (input_element_type == TEXTOID)) 
                    {
                        if ((jsonb_iterator_value.val.string.len == VARSIZE_ANY_EXHDR(input_text)) &&
                            (memcmp(jsonb_iterator_value.val.string.val,
                                VARDATA_ANY(input_text),
                                jsonb_iterator_value.val.string.len) == 0))
                        {
                            if (jsonb_iterator_token == WJB_ELEM)
                                push = false;
                            else
                                skip_level = nest_level;
                        }
                    }
                }
            /* switch end */
        }

        if (push && (skip_level == 0 || nest_level < skip_level))
        {
            return_jsonb_value = pushJsonbValueBlind(&state, jsonb_iterator_token, &jsonb_iterator_value);
        }

        switch (jsonb_iterator_token)
        {
            case WJB_END_OBJECT:
                nest_level--;
                if (skip_level == nest_level && array_level == 0)
                    skip_level = 0;
                break;
            case WJB_END_ARRAY:
                array_level--;
                if (skip_level == nest_level && array_level == 0)
                    skip_level = 0;
            break;
            case WJB_VALUE:
                if (skip_level == nest_level)
                    skip_level = 0;
            /* switch end */
        }
    }

    if (JB_ROOT_IS_SCALAR(input_jsonb) && !return_jsonb_value->val.array.rawScalar && return_jsonb_value->val.array.nElems == 1)
        return_jsonb_value->val.array.rawScalar = true;

    PG_FREE_IF_COPY(input_jsonb, 0);
    if (input_element_type == TEXTOID)
        PG_FREE_IF_COPY(input_text, 1);

    PG_RETURN_JSONB(JsonbValueToJsonb(return_jsonb_value));
}

Datum jsonb_delete_keys(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_delete_keys);

/*
 * Operator function to delete keys from left operand where a match is found in
 * the right operand.
 *
 * jsonb, text[] -> jsonb
 *
 */
Datum 
jsonb_delete_keys(PG_FUNCTION_ARGS)
{
    /* general loops */
    int                     i;

    /* pointers to incoming jsonb and text[] data */
    Jsonb                  *input_jsonb = PG_GETARG_JSONB(0);
    ArrayType              *input_array = PG_GETARG_ARRAYTYPE_P(1);
    int16                   array_typlen;
    bool                    array_typbyval;
    char                    array_typalign;
    Oid                     array_element_type = ARR_ELEMTYPE(input_array); 
    
    /* pointers to return jsonb value data and state to be converted to jsonb on return */
    JsonbParseState        *state = NULL;
    JsonbValue             *return_jsonb_value = NULL;

    /* pointer to iterator for input_jsonb value data */
    JsonbIterator          *jsonb_iterator;
    JsonbValue              jsonb_iterator_value;
    int32                   jsonb_iterator_token;

    /* variables used for skip logic */
    int32                   skip_level = 0;
    int32                   nest_level = 0;
    int32                   array_level = 0;
    bool                    push = true;

    /* array element variables for use during deconstruction */
    Datum                  *datums;
    bool                   *nulls;
    int32                   count;

    /* individual array values values from incoming text[] */
    text                    *array_element_text;
    Numeric                 array_element_numeric = NULL;
    bool                    array_element_bool = false;

    /* ? */ 
    if (array_element_type != TEXTOID && array_element_type != NUMERICOID && array_element_type != BOOLOID)
        elog(ERROR, "invalid data type input");

    /* check input_array is one-dimensional */
    if (ARR_NDIM(input_array) > 1)
        ereport(ERROR, 
            (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                errmsg("1 dimensional text array expected")));

    /* deconstruct array elements */
    get_typlenbyvalalign(array_element_type, &array_typlen, &array_typbyval, &array_typalign);
    deconstruct_array(input_array, array_element_type, array_typlen, array_typbyval, array_typalign,
                       &datums, &nulls, &count);

    /* if the array is empty there's no work to do so return the input value */ 
    if (count == 0) 
        PG_RETURN_JSONB(input_jsonb);

    /*
    * If we've been supplied with existing keys iterate round json data and rebuild 
    * with keys/elements excluded.
    *
    * skip_level, nest_level and array_level are crude counts to check if the the value 
    * for the key is closed and ensure we don't match on keys within nested objects.  
    * Because we are recursing into nested elements but blindly just pushing them onto 
    * the return value we can get away without deeper knowledge of the json?
    */
    jsonb_iterator = JsonbIteratorInit(&input_jsonb->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, false)) != WJB_DONE) {

        push = true; 
        switch (jsonb_iterator_token)
        {
            case WJB_BEGIN_ARRAY:
                array_level++;
                break;
            case WJB_BEGIN_OBJECT:
                nest_level++;
                break;
            case WJB_ELEM:
                if (skip_level == 0 && nest_level == 0 && array_level == 1)
                {
                    if ((jsonb_iterator_value.type == jbvNumeric) && (array_element_type == NUMERICOID))
                    {
                        for (i=0; i<count; i++)
                        {
                            if (!nulls[i]) 
                            {
                                array_element_numeric = DatumGetNumeric(datums[i]);
    
                                if (DatumGetBool(DirectFunctionCall2(numeric_eq,
                                    PointerGetDatum(jsonb_iterator_value.val.numeric),
                                        PointerGetDatum(array_element_numeric))))
                                {
                                    push = false;
                                    break;
                                }
                            }
                        }
                    }
                    else if ((jsonb_iterator_value.type == jbvBool) && (array_element_type == BOOLOID))
                    {
                        for (i=0; i<count; i++)
                        {
                            if (!nulls[i])
                            {
                                array_element_bool = DatumGetBool(datums[i]);

                                if (jsonb_iterator_value.val.boolean == array_element_bool)
                                {
                                    push = false;
                                    break;
                                }
                            }
                        }
                    }
                    /* sql null <> jsonb null */
                }
            case WJB_KEY:
                if (skip_level == 0 && ((jsonb_iterator_token == WJB_KEY && nest_level == 1 && array_level == 0) ||
                    (jsonb_iterator_token == WJB_ELEM && nest_level == 0 && array_level == 1)))
                {
                    if (jsonb_iterator_value.type == jbvString && array_element_type == TEXTOID)
                    {
                        for (i=0; i<count; i++)
                        {
                            if (!nulls[i]) 
                            {
                                array_element_text = DatumGetTextP(datums[i]);
    
                                /* Nulls within json are equal, but should not be equal to SQL nulls */
                                if ((jsonb_iterator_value.val.string.len == VARSIZE_ANY_EXHDR(array_element_text)) &&
                                    (memcmp(jsonb_iterator_value.val.string.val,
                                        VARDATA_ANY(array_element_text),
                                            jsonb_iterator_value.val.string.len) == 0))
                                {
                                    if (jsonb_iterator_token == WJB_ELEM)
                                        push = false;
                                    else
                                        skip_level = nest_level;
                                    break;
                                }
                            }
                        }
                    }
                }
            /* switch end */
        }

        if (push && (skip_level == 0 || nest_level < skip_level))
        {
            return_jsonb_value = pushJsonbValueBlind(&state, jsonb_iterator_token, &jsonb_iterator_value);
        }

        switch (jsonb_iterator_token)
        {
           case WJB_END_OBJECT:
                nest_level--;
                if (skip_level == nest_level && array_level == 0)
                    skip_level = 0;
                break;
            case WJB_END_ARRAY:
                array_level--;
                if (skip_level == nest_level && array_level == 0)
                    skip_level = 0;
                break;
            case WJB_VALUE:
                if (skip_level == nest_level)
                    skip_level = 0;
            /* switch end */
        } 
    }

    if (JB_ROOT_IS_SCALAR(input_jsonb) && !return_jsonb_value->val.array.rawScalar && return_jsonb_value->val.array.nElems == 1)
        return_jsonb_value->val.array.rawScalar = true;

    PG_FREE_IF_COPY(input_jsonb, 0); 
    PG_FREE_IF_COPY(input_array, 1); 

    PG_RETURN_JSONB(JsonbValueToJsonb(return_jsonb_value));
}

Datum jsonb_delete_jsonb(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_delete_jsonb);

/*
 * Operator function to delete keys and values from left operand where a match 
 * is found in the right operand.
 *
 * jsonb, jsonb -> jsonb
 *
 */
Datum 
jsonb_delete_jsonb(PG_FUNCTION_ARGS)
{
    /* pointers to incoming jsonb and text[] data */
    Jsonb                  *input_jsonb_a = PG_GETARG_JSONB(0);
    Jsonb                  *input_jsonb_b = PG_GETARG_JSONB(1);
    
    /* pointers to return jsonb value data and state to be converted to jsonb on return */
    JsonbValue             *return_jsonb_value = NULL;
    JsonbParseState        *state = NULL;

    /* pointer to iterator for input_jsonb_a and temporary value data */
    JsonbIterator          *jsonb_iterator;
    JsonbIterator          *jsonb_iterator2;
    JsonbValue              jsonb_iterator_value;
    JsonbValue              jsonb_iterator_value2;
    JsonbValue              jsonb_iterator_key;
    int32                   jsonb_iterator_token;
    int32                   jsonb_iterator_token2;
    
    bool push = true;

    /* pointer to lookup on input_jsonb_b */
    JsonbValue             *jsonb_lookup_value = NULL;
    
    /*
     * check if either right jsonb is empty and return left if so
     */
    if (JB_ROOT_COUNT(input_jsonb_b) == 0)
        PG_RETURN_JSONB(input_jsonb_a);

    jsonb_iterator = JsonbIteratorInit(&input_jsonb_a->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, true)) != WJB_DONE)
    {
        //skip_nested = true;
        push = true;

        switch (jsonb_iterator_token)
        {
            case WJB_BEGIN_ARRAY:
            case WJB_BEGIN_OBJECT:
            case WJB_END_ARRAY:
            case WJB_END_OBJECT:
                return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, NULL);
                break;
            case WJB_ELEM:
                switch (jsonb_iterator_value.type)
                {
                    case jbvBinary:
                        jsonb_iterator2 = JsonbIteratorInit(&input_jsonb_b->root);
                        while ((jsonb_iterator_token2 = JsonbIteratorNext(&jsonb_iterator2, &jsonb_iterator_value2, true)) != WJB_DONE)
                        {
                            if (jsonb_iterator_value2.type == jbvBinary) 
                            {
                                if ((jsonb_iterator_value2.val.binary.len == jsonb_iterator_value.val.binary.len) &&
                                    (memcmp(jsonb_iterator_value2.val.binary.data, 
                                        jsonb_iterator_value.val.binary.data, 
                                            jsonb_iterator_value2.val.binary.len) == 0))
                                {
                                    push = false;
                                    break;
                                }
                            }
                        }
                    break;
                    default:
                        if (findJsonbValueFromContainer(&input_jsonb_b->root,
                            JB_FARRAY, &jsonb_iterator_value) != NULL)
                            push = false;
                    /* inner switch end */
                }

                if (push) 
                {
                    if (jsonb_iterator_value.type == jbvBinary) 
                    {
                        return_jsonb_value = pushJsonbBinary(&state, jsonb_iterator_value.val.binary.data);
                    }
                    else 
                    {
                        return_jsonb_value = pushJsonbValue(&state, WJB_ELEM, &jsonb_iterator_value);
                    }
                }
                break;
            case WJB_KEY:
                jsonb_lookup_value = findJsonbValueFromContainer(&input_jsonb_b->root, JB_FOBJECT, &jsonb_iterator_value);

                jsonb_iterator_key = jsonb_iterator_value;
                jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, true);
                if (jsonb_iterator_token != WJB_VALUE)
                    elog(ERROR, "invalid JsonbIteratorNext (expected WJB_VALUE) rc: %d", jsonb_iterator_token);

                if (jsonb_lookup_value != NULL)
                {
                    if (jsonb_lookup_value->type == jsonb_iterator_value.type) 
                    {
                        switch (jsonb_lookup_value->type) 
                        {
                            /* Nulls within json are equal, but should not be equal to SQL nulls */
                            case jbvNull:
                                push = false;
                                break;
                            case jbvNumeric:
                                if (DatumGetBool(DirectFunctionCall2(numeric_eq, 
                                    PointerGetDatum(jsonb_lookup_value->val.numeric), 
                                        PointerGetDatum(jsonb_iterator_value.val.numeric))))
                                    push = false;
                                break;
                            case jbvString:
                                if ((jsonb_lookup_value->val.string.len == jsonb_iterator_value.val.string.len) &&
                                    (memcmp(jsonb_lookup_value->val.string.val, 
                                        jsonb_iterator_value.val.string.val, 
                                            jsonb_lookup_value->val.string.len) == 0))
                                    push = false;
                                break;
                            case jbvBinary:
                                if ((jsonb_lookup_value->val.binary.len == jsonb_iterator_value.val.binary.len) &&
                                    (memcmp(jsonb_lookup_value->val.binary.data, 
                                        jsonb_iterator_value.val.binary.data, 
                                            jsonb_lookup_value->val.binary.len) == 0))
                                    push = false;
                                break;
                            case jbvBool:
                                if (jsonb_lookup_value->val.boolean == jsonb_iterator_value.val.boolean)
                                    push = false;
                                break;
                            case jbvArray:
                            case jbvObject:
                                /* should not be possible? */
                            default:
                                ereport(ERROR, (errcode(ERRCODE_SUCCESSFUL_COMPLETION), errmsg("unexpected lookup type %i", jsonb_iterator_token)));
                            /* inner switch end */
                        }
                    }
                }

                if (push) 
                {                
                    return_jsonb_value = pushJsonbValue(&state, WJB_KEY, &jsonb_iterator_key);

                    /* if our value is nested binary data, iterate separately pushing each val */
                    if (jsonb_iterator_value.type == jbvBinary) 
                    {
                        return_jsonb_value = pushJsonbBinary(&state, jsonb_iterator_value.val.binary.data);
                    }
                    else 
                    {
                        return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
                    }
                }
                break;
            case WJB_VALUE:
                /* should not be possible */
            default:
                elog(ERROR, "invalid JsonbIteratorNext rc: %d", jsonb_iterator_token);
            /* switch end */ 
        }

    }

    if (JB_ROOT_IS_SCALAR(input_jsonb_a) && !return_jsonb_value->val.array.rawScalar && return_jsonb_value->val.array.nElems == 1)
        return_jsonb_value->val.array.rawScalar = true;

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_jsonb_b, 1); 

    PG_RETURN_JSONB(JsonbValueToJsonb(return_jsonb_value));
}

Datum jsonb_delete_path(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_delete_path);

/*
 * Test
 * jsonb, text[] -> jsonb
 *
 */
Datum 
jsonb_delete_path(PG_FUNCTION_ARGS)
{
    /* pointers to incoming jsonb and text[] data */
    Jsonb                  *input_jsonb_a = PG_GETARG_JSONB(0);
    ArrayType              *input_array = PG_GETARG_ARRAYTYPE_P(1);

    /* pointer to return jsonb data */
    Jsonb                  *return_jsonb = NULL;

    return_jsonb = jsonbModifyPath(input_jsonb_a, input_array, NULL);

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_array, 1); 

    PG_RETURN_JSONB(return_jsonb);
}

Datum jsonb_concat_jsonb(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_concat_jsonb);

/*
 * Operator function to concatenate json from left operand where a match 
 * is found in the right operand.
 *
 * jsonb, jsonb -> jsonb
 *
 */
Datum 
jsonb_concat_jsonb(PG_FUNCTION_ARGS)
{
    /* incoming jsonb data */
    Jsonb                  *input_jsonb_a = PG_GETARG_JSONB(0);
    Jsonb                  *input_jsonb_b = PG_GETARG_JSONB(1);
    
    /* return jsonb value data to be converted to jsonb on return */
    JsonbParseState        *state = NULL;
    JsonbValue             *return_jsonb_value = NULL;

    /* iterator for input_jsonb_b */
    JsonbIterator          *jsonb_iterator;
    JsonbValue              jsonb_iterator_value;
    int32                   jsonb_iterator_token;
    int32                   jsonb_root_open;
    int32                   jsonb_root_close;

    int32                   nest_level = 0;
    bool                    first = true; 

    /*
     * check if either supplied jsonb is empty and return the other if so
     */
    if (JB_ROOT_COUNT(input_jsonb_a) == 0)
        PG_RETURN_JSONB(input_jsonb_b);
    else if (JB_ROOT_COUNT(input_jsonb_b) == 0)
        PG_RETURN_JSONB(input_jsonb_a);

    /* 
     * rather than restrict concatenation to objects, allow any jsonb root
     * but if one is an array use an array as the root container else
     * default to object
     */
    if (JB_ROOT_IS_ARRAY(input_jsonb_a) || JB_ROOT_IS_ARRAY(input_jsonb_b))
    {
        jsonb_root_open = WJB_BEGIN_ARRAY;
        jsonb_root_close = WJB_END_ARRAY; 
    } else
    {
        jsonb_root_open = WJB_BEGIN_OBJECT;
        jsonb_root_close = WJB_END_OBJECT; 
    }

    /*
     * The following is essentially a cut 'n shut job; discarding the closing root 
     * object token from the first jsonb value and the opening one from the second.
     * Values from each are just blindly pushed onto the return value leaving
     * deduplication down to lower level jsonb logic.
     */

    return_jsonb_value = pushJsonbValue(&state, jsonb_root_open, NULL);

    jsonb_iterator = JsonbIteratorInit(&input_jsonb_a->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, false)) != WJB_DONE) 
    {
        if (jsonb_iterator_token == jsonb_root_open && (first || nest_level > 0)) 
        {
            nest_level++;
            if (nest_level == 1)
            { 
                first = false;
                continue;
            }
        }
        else if (jsonb_iterator_token == jsonb_root_close && nest_level > 0) 
        {
            nest_level--;
            if (nest_level == 0) 
                continue;
        }

        first = false;
        return_jsonb_value = pushJsonbValueBlind(&state, jsonb_iterator_token, &jsonb_iterator_value);
    }

    nest_level = 0;
    first = true;
    jsonb_iterator = JsonbIteratorInit(&input_jsonb_b->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, false)) != WJB_DONE)
    {
        if (jsonb_iterator_token == jsonb_root_open && (first || nest_level > 0)) 
        {
            nest_level++;
            if (nest_level == 1) 
            {
                first = false;
                continue;
            }
        }
        else if (jsonb_iterator_token == jsonb_root_close && nest_level > 0) 
        {
            nest_level--;
            if (nest_level == 0) 
                continue;
        }

        first = false;
        return_jsonb_value = pushJsonbValueBlind(&state, jsonb_iterator_token, &jsonb_iterator_value);
    }

    return_jsonb_value = pushJsonbValue(&state, jsonb_root_close, NULL);

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_jsonb_b, 1); 

    PG_RETURN_JSONB(JsonbValueToJsonb(return_jsonb_value));
}

Datum jsonb_replace_jsonb(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_replace_jsonb);

/*
 * Operator function to replace json in left operand where keys match 
 * in the right operand.
 *
 * jsonb, jsonb -> jsonb
 *
 */
Datum 
jsonb_replace_jsonb(PG_FUNCTION_ARGS)
{
    /* incoming jsonb data */
    Jsonb                  *input_jsonb_a = PG_GETARG_JSONB(0);
    Jsonb                  *input_jsonb_b = PG_GETARG_JSONB(1);
    
    /* return jsonb value data to be converted to jsonb on return */
    JsonbParseState        *state = NULL;
    JsonbValue             *return_jsonb_value = NULL;

    /* lookup jsonb value data */
    JsonbValue              jsonb_lookup_key;
    JsonbValue             *jsonb_lookup_value = NULL;
    uint32                  jsonb_lookup_flags;

    /* iterator for input_jsonb_b */
    JsonbIterator          *jsonb_iterator;
    JsonbValue              jsonb_iterator_value;
    int32                   jsonb_iterator_token;

    /*
     * check if  supplied replacement jsonb is empty and return unchanged if so
     */
    if (JB_ROOT_COUNT(input_jsonb_b) == 0)
        PG_RETURN_JSONB(input_jsonb_a);

    if (JB_ROOT_IS_OBJECT(input_jsonb_a))
        jsonb_lookup_flags = JB_FOBJECT;
    else
        jsonb_lookup_flags = JB_FOBJECT | JB_FARRAY;

    jsonb_iterator = JsonbIteratorInit(&input_jsonb_a->root);
    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, true)) != WJB_DONE) 
    {
        if ((jsonb_iterator_token == WJB_ELEM ) && (jsonb_iterator_value.type == jbvBinary))
        {
            return_jsonb_value = pushJsonbBinary(&state, jsonb_iterator_value.val.binary.data);
        }
        else
        {
            return_jsonb_value = pushJsonbValueBlind(&state, jsonb_iterator_token, &jsonb_iterator_value);
        }

        Assert(jsonb_iterator_token != WJB_VALUE);

        if ( jsonb_iterator_token == WJB_KEY )
        {
            jsonb_lookup_key.type = jbvString;
            jsonb_lookup_key.val.string.val = jsonb_iterator_value.val.string.val;
            jsonb_lookup_key.val.string.len = jsonb_iterator_value.val.string.len;

            jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, true);
            Assert(jsonb_iterator_token == WJB_VALUE);

            jsonb_lookup_value = findJsonbValueFromContainer(&input_jsonb_b->root,
                jsonb_lookup_flags, &jsonb_lookup_key);

            /* if there's nothing to replace push the original value */
            if (jsonb_lookup_value == NULL) 
            {
                jsonb_lookup_value = &jsonb_iterator_value;
            }

            /* if our value is nested binary data, iterate separately pushing each val */
            if (jsonb_lookup_value->type == jbvBinary)
            {
                return_jsonb_value = pushJsonbBinary(&state, jsonb_lookup_value->val.binary.data);
            }
            else 
            {
                return_jsonb_value = pushJsonbValue(&state, WJB_VALUE, jsonb_lookup_value);
            }
        }
    }

    if (JB_ROOT_IS_SCALAR(input_jsonb_a) && !return_jsonb_value->val.array.rawScalar && return_jsonb_value->val.array.nElems == 1)
        return_jsonb_value->val.array.rawScalar = true;

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_jsonb_b, 1); 

    PG_RETURN_JSONB(JsonbValueToJsonb(return_jsonb_value));
}

Datum jsonb_replace_path(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_replace_path);

/*
 * Test
 * jsonb, text[], jsonb -> jsonb
 *
 */
Datum 
jsonb_replace_path(PG_FUNCTION_ARGS)
{
    /* pointers to incoming jsonb and text[] data */
    Jsonb                  *input_jsonb_a = PG_GETARG_JSONB(0);
    ArrayType              *input_array = PG_GETARG_ARRAYTYPE_P(1);
    Jsonb                  *input_jsonb_b = PG_GETARG_JSONB(2);

    /* pointer to return jsonb data */
    Jsonb                  *return_jsonb = NULL;

    return_jsonb = jsonbModifyPath(input_jsonb_a, input_array, input_jsonb_b);

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_array, 1); 
    PG_FREE_IF_COPY(input_jsonb_b, 2); 

    PG_RETURN_JSONB(return_jsonb);
}


Datum jsonb_append_path(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_append_path);

/*
 * Test
 * jsonb, text[], jsonb -> jsonb
 *
 */
Datum 
jsonb_append_path(PG_FUNCTION_ARGS)
{
    /* pointers to incoming jsonb and text[] data */
    Jsonb                  *input_jsonb_a = PG_GETARG_JSONB(0);
    ArrayType              *input_array = PG_GETARG_ARRAYTYPE_P(1);
    Jsonb                  *input_jsonb_b = PG_GETARG_JSONB(2);

    /* pointer to return jsonb data */
    Jsonb                  *return_jsonb = NULL;

    return_jsonb = jsonbModifyPath(input_jsonb_a, input_array, input_jsonb_b);

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_array, 1); 
    PG_FREE_IF_COPY(input_jsonb_b, 2); 

    PG_RETURN_JSONB(return_jsonb);
}
