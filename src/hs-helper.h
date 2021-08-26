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

#ifndef HOMESCREEN_HELPER_H
#define HOMESCREEN_HELPER_H
#define AFB_BINDING_VERSION 3
#include <afb/afb-binding.h>
#include <json-c/json.h>
#include <string>

#define AFB_EVENT_BAD_REQUEST                 100
#define AFB_REQ_SUBSCRIBE_ERROR               101
#define AFB_REQ_UNSUBSCRIBE_ERROR             102
#define AFB_REQ_SHOWNOTIFICATION_ERROR        103
#define AFB_REQ_SHOWINFORMATION_ERROR         104
#define AFB_REQ_GETAPPLICATIONID_ERROR        105
#define AFB_REQ_NOT_STARTED_APPLICATION       106

typedef enum REQ_ERROR
{
  REQ_FAIL = -1,
  REQ_OK=0,
  NOT_NUMBER,
  OUT_RANGE
}REQ_ERROR;

extern const char* evlist[];
extern const char _error[];
extern const char _application_id[];
extern const char _display_message[];
extern const char _reply_message[];
extern const char _keyData[];
extern const char _keyId[];

REQ_ERROR get_value_uint16(const afb_req_t request, const char *source, uint16_t *out_id);
REQ_ERROR get_value_int16(const afb_req_t request, const char *source, int16_t *out_id);
REQ_ERROR get_value_int32(const afb_req_t request, const char *source, int32_t *out_id);
void hs_add_object_to_json_object(struct json_object* j_obj, int count, ...);
void hs_add_object_to_json_object_str(struct json_object* j_obj, int count, ...);
void hs_add_object_to_json_object_func(struct json_object* j_obj, const char* verb_name, int count, ...);
int hs_search_event_name_index(const char* value);
std::string get_application_id(const afb_req_t request);

typedef int (*event_hook_func)(afb_api_t api, const char *event, struct json_object *object);
void setEventHook(const char *event, const event_hook_func f);

#endif /*HOMESCREEN_HELPER_H*/
