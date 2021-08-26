/*
 * Copyright (c) 2019 TOYOTA MOTOR CORPORATION
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

#ifndef HOMESCREEN_PROXY_H
#define HOMESCREEN_PROXY_H

#include <string>
#include <json-c/json.h>
#include <functional>
#include "hs-helper.h"

struct HS_AfmMainProxy {
    // synchronous call, call result in object
    int runnables(afb_api_t api, struct json_object **object);
    int ps(afb_api_t api, struct json_object **object);
    int detail(afb_api_t api, const std::string &id, struct json_object **object);

    // asynchronous call, reply in callback function
    void start(struct hs_instance *hs_instance, afb_req_t request, const std::string &id);
};

#endif // HOMESCREEN_PROXY_H
