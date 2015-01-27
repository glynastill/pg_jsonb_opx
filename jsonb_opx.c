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
#include "utils/jsonb.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"

#ifdef PG_MODULE_MAGIC
	PG_MODULE_MAGIC;
#endif 

Datum jsonb_delete_text(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(jsonb_delete_text);

/*
 * Operator function to delete keys from left operand where a match is found in
 * the right operand.
 *
 * jsonb, text[] -> jsonb
 *
 */
Datum 
jsonb_delete_text(PG_FUNCTION_ARGS)
{
    /* general loops */
	int i;

    /* pointers to incoming jsonb and text[] data */
    Jsonb *input_jsonb = PG_GETARG_JSONB(0);
	ArrayType *input_array = PG_GETARG_ARRAYTYPE_P(1);
    
    /* pointers to return jsonb_value data and state to be converted to jsonb on return */
    JsonbParseState *state = NULL;
    JsonbValue *return_jsonb_value = NULL;

    /* pointer to iterator for input_jsonb and lookup value data */
    JsonbValue  jsonb_lookup_value;
    JsonbValue *jsonb_value = NULL;
    JsonbIterator *jsonb_iterator;
    JsonbValue  jsonb_iterator_value;
    int32 jsonb_iterator_token;

    /* variables used for skip logic */
    int32 skip_key = 0;
    int32 nest_level = 0;
    int32 array_level = 0;

	/* array element variables for use during deconstruction */
	Datum *datums;
	bool *nulls;
	int32 count;

    /* individual array values values from incoming text[] */
    text *array_element_text;
    bool exists = false;

    /* check that supplied jsonb isn't non object, i.e. scalar or array */
    if (!JB_ROOT_IS_OBJECT(input_jsonb))
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                errmsg("cannot call on a non-object")));

    /* assert input_array is a text array type */
    Assert(ARR_ELEMTYPE(input_array) == TEXTOID);

    /* check input_array is one-dimensional */
    if (ARR_NDIM(input_array) > 1)
        ereport(ERROR, 
            (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                errmsg("1 dimensional text array expected")));

    /* deconstruct array elements */
    deconstruct_array(input_array, TEXTOID, -1, false, 'i', 
                       &datums, &nulls, &count);

    /* first check to make sure at least one key exists - this is potentially just extra unwanted work */
    for (i=0; i<count; i++)
    {
        if (nulls[i])
            continue;

        array_element_text = DatumGetTextP(datums[i]);

        jsonb_lookup_value.type = jbvString;
        jsonb_lookup_value.val.string.val = VARDATA_ANY(array_element_text);
        jsonb_lookup_value.val.string.len = VARSIZE_ANY_EXHDR(array_element_text);
            
        jsonb_value = findJsonbValueFromContainer(&input_jsonb->root,
            JB_FOBJECT | JB_FARRAY, &jsonb_lookup_value);

        if (jsonb_value != NULL) {
            exists = true;
            break;
        }
    }

    if (!exists) 
        PG_RETURN_JSONB(input_jsonb);

    /*
    * If we've been supplied with existing keys iterate round json data matching those keys.
    *
    * skip_key, nest_level and array_level are crude counts to check if the the value for the key is closed
    * and ensure we don't match on keys within nested objects.  Because we are recursing into nested elements
    * but blindly just pushing them onto the return value we can get away without deeper knowledge of the json?
    */

    jsonb_iterator = JsonbIteratorInit(&input_jsonb->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, false)) != WJB_DONE) {

        switch (jsonb_iterator_token)
        {
        case WJB_BEGIN_ARRAY:
            array_level++;
            if (skip_key == 0)
                return_jsonb_value = pushJsonbValue(&state, WJB_BEGIN_ARRAY, &jsonb_iterator_value);
            break;
        case WJB_BEGIN_OBJECT:
            nest_level++;
            if (skip_key == 0)
                return_jsonb_value = pushJsonbValue(&state, WJB_BEGIN_OBJECT, &jsonb_iterator_value);
            break;
        case WJB_KEY:
            /* Check each key against our array of keys */
            if (skip_key > 0) {
                skip_key++;
            }
            else if (nest_level == 1){
                for (i=0; i<count; i++)
                {
                    if (nulls[i])
                        continue;

                    array_element_text = DatumGetTextP(datums[i]);

                    if (strcmp(VARDATA(array_element_text), pnstrdup(jsonb_iterator_value.val.string.val,jsonb_iterator_value.val.string.len)) == 0) {
                        skip_key++;
                        break;
                    }
                }
            }

            if (skip_key == 0)
                return_jsonb_value = pushJsonbValue(&state, WJB_KEY, &jsonb_iterator_value);
            break;
        case WJB_VALUE:
            if (skip_key == 0)
                return_jsonb_value = pushJsonbValue(&state, WJB_VALUE, &jsonb_iterator_value);
            else if (skip_key > 0)
                skip_key--;
            break;
        case WJB_ELEM:
            if (skip_key == 0)
                return_jsonb_value = pushJsonbValue(&state, WJB_ELEM, &jsonb_iterator_value);
            break;
        case WJB_END_ARRAY:
            array_level--;
            if (skip_key == 0)
                return_jsonb_value = pushJsonbValue(&state, WJB_END_ARRAY, NULL);
            else if (skip_key > 0 && array_level == 0)
                skip_key--;
            break;
        case WJB_END_OBJECT:
            nest_level--;
            if (skip_key == 0)
                return_jsonb_value = pushJsonbValue(&state, WJB_END_OBJECT, NULL);
            else if (skip_key > 0)
                skip_key--;
            break;
        default:
            elog(ERROR, "invalid JsonbIteratorNext rc: %d", jsonb_iterator_token);
        } 
    }
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
    Jsonb *input_jsonb_a = PG_GETARG_JSONB(0);
    Jsonb *input_jsonb_b = PG_GETARG_JSONB(1);
    
    /* pointers to return jsonb_value data and state to be converted to jsonb on return */
    JsonbValue *return_jsonb_value = NULL;
    JsonbParseState *state = NULL;

    /* pointer to iterator for input_jsonb_a and temporary value data */
    JsonbIterator *jsonb_iterator;
    JsonbValue  jsonb_iterator_value;
    JsonbValue  jsonb_iterator_key;
    int32 jsonb_iterator_token;
    bool skip_nested = false;
    
    /* pointer to iterator and container for pushing nested parts of input_jsonb_a */
    JsonbContainer *nest_jsonb_container_a;
    JsonbIterator *nest_jsonb_iterator;
    bool push = true;

    /* inner iterator for iterating around jbvBinary types */
    JsonbValue  nest_jsonb_iterator_value;
    int32 nest_jsonb_iterator_token;

    /* pointer to lookup on input_jsonb_b */
    JsonbValue *jsonb_lookup_value = NULL;
    
    jsonb_iterator = JsonbIteratorInit(&input_jsonb_a->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, skip_nested)) != WJB_DONE) {
        skip_nested = true;

        switch (jsonb_iterator_token)
        {
        case WJB_BEGIN_ARRAY:
        case WJB_BEGIN_OBJECT:
        case WJB_ELEM:
        case WJB_END_ARRAY:
        case WJB_END_OBJECT:
            return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
        break;
        case WJB_KEY:
             jsonb_lookup_value = findJsonbValueFromContainer(&input_jsonb_b->root, JB_FOBJECT | JB_FARRAY, &jsonb_iterator_value);
             jsonb_iterator_key = jsonb_iterator_value;

             jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, skip_nested);
             if (jsonb_iterator_token != WJB_VALUE)
                 elog(ERROR, "invalid JsonbIteratorNext (expected WJB_VALUE) rc: %d", jsonb_iterator_token);

             if (jsonb_lookup_value != NULL) {
                if (jsonb_lookup_value->type == jsonb_iterator_value.type) {
                    switch (jsonb_lookup_value->type) {
                        case jbvNumeric:
                            if (strcmp(
                                DatumGetCString(DirectFunctionCall1(numeric_out, PointerGetDatum(jsonb_lookup_value->val.numeric))),
                                DatumGetCString(DirectFunctionCall1(numeric_out, PointerGetDatum(jsonb_iterator_value.val.numeric)))
                                ) == 0) 
                                push = false;
                        break;
                        case jbvString:
                            if (strcmp(
                                pnstrdup(jsonb_lookup_value->val.string.val,jsonb_lookup_value->val.string.len),
                                pnstrdup(jsonb_iterator_value.val.string.val,jsonb_iterator_value.val.string.len)
                                ) == 0)
                                push = false;
                        break;
                        case jbvBinary:
                            if (strcmp(
                                JsonbToCString(NULL, jsonb_lookup_value->val.binary.data, jsonb_lookup_value->val.binary.len),
                                JsonbToCString(NULL, jsonb_iterator_value.val.binary.data, jsonb_lookup_value->val.binary.len)
                                ) == 0)
                                push = false;
                        break;
                        case jbvBool:
                            if (jsonb_lookup_value->val.boolean == jsonb_iterator_value.val.boolean)
                                push = false;
                        break;
                        case jbvArray:
                        /* should not be possible? */
                        case jbvObject:
                        /* should not be possible? */
                        default:
                        ereport(NOTICE, (errcode(ERRCODE_SUCCESSFUL_COMPLETION), errmsg("unexpected lookup type")));
                    }
                }
            }

            if (push) {                
                return_jsonb_value = pushJsonbValue(&state, WJB_KEY, &jsonb_iterator_key);

                /* if our value is nested binary data, iterate separately pushing each val */
                if (jsonb_iterator_value.type == jbvBinary) {
                    nest_jsonb_container_a = jsonb_iterator_value.val.binary.data;

                    nest_jsonb_iterator = JsonbIteratorInit(nest_jsonb_container_a);
                    while ((nest_jsonb_iterator_token = JsonbIteratorNext(&nest_jsonb_iterator, &nest_jsonb_iterator_value, false)) != WJB_DONE) {
                        return_jsonb_value = pushJsonbValue(&state, nest_jsonb_iterator_token, &nest_jsonb_iterator_value);
                    }
                }
                else {
                    return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
                }
            }
            else 
                push = true;
            break;
        case WJB_VALUE:
            /* should not be possible */
        default:
            elog(ERROR, "invalid JsonbIteratorNext rc: %d", jsonb_iterator_token);
        }

    }

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_jsonb_b, 1); 

    PG_RETURN_JSONB(JsonbValueToJsonb(return_jsonb_value));
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
    /* incoming jsonb and text[] data */
    Jsonb *input_jsonb_a = PG_GETARG_JSONB(0);
    Jsonb *input_jsonb_b = PG_GETARG_JSONB(1);
    
    /* return jsonb_value data to be converted to jsonb on return */
    JsonbValue *return_jsonb_value = NULL;

    /* iterator for input_jsonb_b */
    JsonbIterator *jsonb_iterator;
    JsonbValue  jsonb_iterator_value;
    int32 jsonb_iterator_token;

    JsonbParseState *state = NULL;
    bool skip_nested = false;

    int32 nest_level = 0;
   
    /* check that supplied jsonb isn't non object, i.e. scalar or array */
    if (!JB_ROOT_IS_OBJECT(input_jsonb_a) || !JB_ROOT_IS_OBJECT(input_jsonb_b))
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                errmsg("cannot call on a non-object")));

    /*
     * The following is essentially a cut 'n shut job; discarding the closing root 
     * object token from the first jsonb value and the opening one from the second.
     * Values from each are just blindly pushed onto the return value leaving
     * deduplication down to lower level jsonb logic.
     */

    jsonb_iterator = JsonbIteratorInit(&input_jsonb_a->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, skip_nested)) != WJB_DONE) {

        switch (jsonb_iterator_token)
        {
        case WJB_BEGIN_ARRAY:
        case WJB_KEY:
        case WJB_VALUE:
        case WJB_ELEM:
        case WJB_END_ARRAY:
            return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
            break;
        case WJB_BEGIN_OBJECT:
            return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
            nest_level++;
            break;
        case WJB_END_OBJECT:
            nest_level--;
            if (nest_level != 0)
                return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
            break;
        default:
            elog(ERROR, "invalid JsonbIteratorNext rc: %d", jsonb_iterator_token);
        }

    }

    nest_level = 0;
    jsonb_iterator = JsonbIteratorInit(&input_jsonb_b->root);

    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, skip_nested)) != WJB_DONE) {

        switch (jsonb_iterator_token)
        {
        case WJB_BEGIN_ARRAY:
        case WJB_KEY:
        case WJB_VALUE:
        case WJB_ELEM:
        case WJB_END_ARRAY:
            return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
            break;
        case WJB_BEGIN_OBJECT:
            if (nest_level != 0)
                return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
            nest_level++;
            break;
        case WJB_END_OBJECT:
            nest_level--;
            return_jsonb_value = pushJsonbValue(&state, jsonb_iterator_token, &jsonb_iterator_value);
            break;
        default:
            elog(ERROR, "invalid JsonbIteratorNext rc: %d", jsonb_iterator_token);
        }

    }

    PG_FREE_IF_COPY(input_jsonb_a, 0); 
    PG_FREE_IF_COPY(input_jsonb_b, 1); 

    PG_RETURN_JSONB(JsonbValueToJsonb(return_jsonb_value));
}
