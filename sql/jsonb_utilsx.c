/*
 * jsonb_utilsx.c
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
#include "utils/jsonb.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "jsonb_opx.h"

JsonbValue * 
pushJsonbBinary(JsonbParseState **pstate, JsonbContainer *jsonb_container)
{
    JsonbIterator *jsonb_iterator;
    JsonbValue jsonb_iterator_value;
    int32 jsonb_iterator_token;
    JsonbValue *return_jsonb_value = NULL;

    jsonb_iterator = JsonbIteratorInit((void *)jsonb_container);
    while ((jsonb_iterator_token = JsonbIteratorNext(&jsonb_iterator, &jsonb_iterator_value, false)) != WJB_DONE)
    {
        return_jsonb_value = pushJsonbValueBlind(pstate, jsonb_iterator_token, &jsonb_iterator_value);
    }
    return return_jsonb_value;
}

JsonbValue *
pushJsonbValueBlind(JsonbParseState **pstate, JsonbIteratorToken jsonb_iterator_token, JsonbValue *jsonb_iterator_value)
{
    JsonbValue *return_jsonb_value = NULL;

    if ((jsonb_iterator_token == WJB_KEY) ||
            (jsonb_iterator_token == WJB_VALUE) ||
            (jsonb_iterator_token == WJB_ELEM) ||
            (jsonb_iterator_token == WJB_BEGIN_ARRAY && jsonb_iterator_value->val.array.rawScalar))
            {
            return_jsonb_value = pushJsonbValue(pstate, jsonb_iterator_token, jsonb_iterator_value);
            }
        else
            return_jsonb_value = pushJsonbValue(pstate, jsonb_iterator_token, NULL);

    return return_jsonb_value;
}

Jsonb * 
jsonbModifyPath(Jsonb *jsonb_a, ArrayType *array_path, Jsonb *jsonb_b)
{
    /* pointers to return jsonb value data and state to be converted to jsonb on return */
    JsonbParseState *state = NULL;
    JsonbValue *return_jsonb_value = NULL;

    /* pointer to iterator for input jsonb */
    JsonbIterator *jsonb_iterator;
    JsonbValue jsonb_iterator_value;
    int32 jsonb_iterator_token;
    int32 jsonb_last_token = 0;

    JsonbIterator *jsonb_replacement_iterator;
    JsonbValue jsonb_replacement_iterator_value;
    int32 jsonb_replacement_iterator_token;
    
    /* 
     * array element variables for use during deconstruction 
     * count is the depth we will be looking from the first matching key
     */
    Datum *datums;
    bool *nulls;
    int32 count; 

    /* the current key we are looking for, starting with the first key */
    text *key_on = NULL;
    int32 index_on = 0; 
    int32 nest_level = 0;
    int32 array_level = 0;
    int32 skip_level = 0;
    int32 push_nest_level = 0;
    bool push = true;

    /* assert input_array is a text array type */
    Assert(ARR_ELEMTYPE(array_path) == TEXTOID);

    /* check input_array is one-dimensional */
    if (ARR_NDIM(array_path) > 1)
        ereport(ERROR,
            (errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
                errmsg("1 dimensional text array expected")));

    /* deconstruct array elements */
    deconstruct_array(array_path, TEXTOID, -1, false, 'i',
                        &datums, &nulls, &count);

    /* can't follow key based paths on non objects */ 
    if (!JB_ROOT_IS_OBJECT(jsonb_a) && (count > 1))
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                errmsg("cannot call with path deeper than 1 on a non-object")));

     /* if the array is empty there's no work to do so return the input value */
    if ((count == 0) || (JB_ROOT_COUNT(jsonb_a) == 0))
        return jsonb_a;

    if (!nulls[index_on])
        key_on = DatumGetTextP(datums[index_on]);

    jsonb_iterator = JsonbIteratorInit(&jsonb_a->root);

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
            case WJB_KEY:
                /*
                 * If we are not skipping the current nest level check that the nesting level follows the array index
                 * and if it does check the current key. For array elements only check the root level (array_level==1).
                 * If there is no match we just keep on pushing
                 */
                if (skip_level == 0 && ((jsonb_iterator_token == WJB_KEY && nest_level-1 == index_on && array_level == 0) ||
                        (jsonb_iterator_token == WJB_ELEM && nest_level == 0 && array_level == 1)))
                {
                    if ((jsonb_iterator_value.type == jbvNull && key_on == NULL) ||
                        (key_on != NULL && (jsonb_iterator_value.val.string.len == VARSIZE_ANY_EXHDR(key_on)) &&
                        (memcmp(jsonb_iterator_value.val.string.val, VARDATA_ANY(key_on), jsonb_iterator_value.val.string.len) == 0)))
                    {   
                        /* if we have not yet reached the last index in the array / key chain move on and check the next */
                        if (index_on < count-1) 
                        {
                            index_on++;
                            if (!nulls[index_on])
                                key_on = DatumGetTextP(datums[index_on]);
                            else
                                key_on = NULL;
                        }
                        /* if we have reached the last index, the we can modify this level */ 
                        else 
                        {
                            /* if jsonb_b is not null unwrap it with iterator into replacement_jsonb_value */
                            if (jsonb_b != NULL) {
                                jsonb_replacement_iterator = JsonbIteratorInit(&jsonb_b->root);

                                while ((jsonb_replacement_iterator_token = JsonbIteratorNext(&jsonb_replacement_iterator, &jsonb_replacement_iterator_value, false)) != WJB_DONE)
                                {
                                    if (((jsonb_last_token == jsonb_replacement_iterator_token) && 
                                        (jsonb_last_token != WJB_VALUE)) || 
                                        ((jsonb_last_token == WJB_VALUE) && 
                                        ((jsonb_replacement_iterator_token == WJB_BEGIN_OBJECT) || 
                                        (jsonb_replacement_iterator_token == WJB_BEGIN_ARRAY)))) 
                                    {
                                        push_nest_level++;
                                    }

                                    if ((jsonb_replacement_iterator_token == WJB_KEY) || 
                                        (jsonb_replacement_iterator_token == WJB_VALUE) || 
                                        (jsonb_replacement_iterator_token == WJB_ELEM) || (push_nest_level != 1))
                                    {
                                        return_jsonb_value = pushJsonbValueBlind(&state, jsonb_replacement_iterator_token, &jsonb_replacement_iterator_value);
                                    }

                                    if (((jsonb_last_token == WJB_BEGIN_ARRAY) || 
                                        (jsonb_last_token == WJB_VALUE)) && 
                                        (jsonb_replacement_iterator_token == WJB_END_ARRAY))
                                    {
                                        push_nest_level--;
                                    }
                                    else if (((jsonb_last_token == WJB_BEGIN_OBJECT) || 
                                        (jsonb_last_token == WJB_VALUE)) && 
                                        (jsonb_replacement_iterator_token == WJB_END_OBJECT))
                                    {
                                        push_nest_level--;
                                    }
                                }
                            }
                            if (jsonb_iterator_token == WJB_ELEM) 
                                push = false;
                            else
                                skip_level = nest_level;
                        }
                    } 
                }
        }

        if (push && (skip_level == 0 || nest_level < skip_level)) 
        {
            return_jsonb_value = pushJsonbValueBlind(&state, jsonb_iterator_token, &jsonb_iterator_value);
            jsonb_last_token = jsonb_iterator_token;
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
        }
    }

    if (return_jsonb_value->type == jbvArray && return_jsonb_value->val.array.rawScalar && return_jsonb_value->val.array.nElems == 0)
    {
        return_jsonb_value->val.array.rawScalar = false;
    }

    return JsonbValueToJsonb(return_jsonb_value);
    
}
