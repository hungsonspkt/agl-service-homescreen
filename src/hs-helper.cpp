/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <cstdarg>
#include "hs-helper.h"


const char* evlist[] = {
    "tap_shortcut",
    "on_screen_message",
    "on_screen_reply",
    "showWindow",
    "hideWindow",
    "replyShowWindow",
    "showNotification",
    "showInformation",
    "application-list-changed",
    "reserved"
  };

/**
 * get uint16 value from source
 *
 * #### Parameters
 * - request : Describes the request by bindings from afb-daemon
 * - source  : input source
 * - out_id  : output uint16 value
 *
 * #### Return
 * error code
 *
 */
REQ_ERROR get_value_uint16(const afb_req_t request, const char *source, uint16_t *out_id)
{
    char* endptr;
    const char* tmp = afb_req_value (request, source);
    if(!tmp)
    {
        return REQ_FAIL;
    }
    long tmp_id = strtol(tmp,&endptr,10);

    /* error check of range */
    if( (tmp_id > UINT16_MAX) || (tmp_id < 0) )
    {
        return OUT_RANGE;
    }
    if(*endptr != '\0')
    {
        return NOT_NUMBER;
    }

    *out_id = (uint16_t)tmp_id;
    return REQ_OK;
}

/**
 * get int16 value from source
 *
 * #### Parameters
 * - request : Describes the request by bindings from afb-daemon
 * - source  : input source
 * - out_id  : output int16 value
 *
 * #### Return
 * error code
 *
 */
REQ_ERROR get_value_int16(const afb_req_t request, const char *source, int16_t *out_id)
{
    char* endptr;
    const char* tmp = afb_req_value (request, source);
    if(!tmp)
    {
        return REQ_FAIL;
    }
    long tmp_id = strtol(tmp,&endptr,10);

    /* error check of range */
    if( (tmp_id > INT16_MAX) || (tmp_id < INT16_MIN) )
    {
        return OUT_RANGE;
    }
    if(*endptr != '\0')
    {
        return NOT_NUMBER;
    }

    *out_id = (int16_t)tmp_id;
    return REQ_OK;
}

/**
 * get int32 value from source
 *
 * #### Parameters
 * - request : Describes the request by bindings from afb-daemon
 * - source  : input source
 * - out_id  : output int32 value
 *
 * #### Return
 * error code
 *
 */
REQ_ERROR get_value_int32(const afb_req_t request, const char *source, int32_t *out_id)
{
    char* endptr;
    const char* tmp = afb_req_value (request, source);
    if(!tmp)
    {
        return REQ_FAIL;
    }
    long tmp_id = strtol(tmp,&endptr,10);

    /* error check of range */
    if( (tmp_id > INT32_MAX) || (tmp_id < INT32_MIN) )
    {
        return OUT_RANGE;
    }
    if(*endptr != '\0')
    {
        return NOT_NUMBER;
    }

    *out_id = (int32_t)tmp_id;
    return REQ_OK;
}

/**
 * add int object to json object
 *
 * #### Parameters
 * - j_obj : the json object will join in int json object
 * - count : input parameter number
 * - ...   : parameter list
 *
 * #### Return
 * None
 *
 */
void hs_add_object_to_json_object(struct json_object* j_obj, int count,...)
{
    va_list args;
    va_start(args, count);
    for(int i = 0; i < count; ++i )
    {
        char *key = va_arg(args, char*);
        int value = va_arg(args, int);
        json_object_object_add(j_obj, key, json_object_new_int((int32_t)value));
        ++i;
    }
    va_end(args);
}

/**
 * add string object to json object
 *
 * #### Parameters
 * - j_obj : the json object will join in string json object
 * - count : input parameter number
 * - ...   : parameter list
 *
 * #### Return
 * None
 *
 */
void hs_add_object_to_json_object_str(struct json_object* j_obj, int count,...)
{
    va_list args;
    va_start(args, count);
    for(int i = 0; i < count; ++i )
    {
        char *key = va_arg(args, char*);
        char *value = va_arg(args, char*);
        json_object_object_add(j_obj, key, json_object_new_string(value));
        ++i;
    }
    va_end(args);
}

/**
 * add new json object to json object
 *
 * #### Parameters
 * - j_obj : the json object will join in new json object
 * - verb_name : new json object's verb value
 * - count : input parameter number
 * - ...   : parameter list
 *
 * #### Return
 * None
 *
 */
void hs_add_object_to_json_object_func(struct json_object* j_obj, const char* verb_name, int count, ...)
{
    va_list args;
    va_start(args, count);

    json_object_object_add(j_obj,"verb", json_object_new_string(verb_name));

    for(int i = 0; i < count; ++i )
    {
        char *key = va_arg(args, char*);
        int value = va_arg(args, int);
        json_object_object_add(j_obj, key, json_object_new_int((int32_t)value));
        ++i;
    }
    va_end(args);
}

/**
 * search event position in event list
 *
 * #### Parameters
 * - value : searched event name
 *
 * #### Return
 * event's index in event list
 *
 */
int hs_search_event_name_index(const char* value)
{
    size_t buf_size = 50;
    size_t size = sizeof evlist / sizeof *evlist;
    int ret = -1;
    for(size_t i = 0 ; i < size ; ++i)
    {
        if(!strncmp(value, evlist[i], buf_size))
        {
            ret = i;
            break;
        }
    }
    return ret;
}

/**
 * get application id from request
 *
 * #### Parameters
 * - request : the request
 *
 * #### Return
 * got application id
 *
 */
std::string get_application_id(const afb_req_t request)
{
    std::string appid;
    char *app_id = afb_req_get_application_id(request);
    if(app_id == nullptr) {
        appid = std::string("");
    }
    else {
        appid = std::string(app_id);
        free(app_id);
    }

    return appid;
}
