/*
 * Copyright (c) 2018 TOYOTA MOTOR CORPORATION
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

#ifndef HOMESCREEN_CLIENT_H
#define HOMESCREEN_CLIENT_H

#include <string>
#include <unordered_set>
#include <unordered_map>
#include "hs-helper.h"


class HS_Client {
public:
    HS_Client(afb_req_t request, const char* id) : HS_Client(request, std::string(id)){}
    HS_Client(afb_req_t request, std::string id);
    HS_Client(HS_Client&) = delete;
    HS_Client &operator=(HS_Client&) = delete;
    ~HS_Client();

    int handleRequest(afb_req_t request, const char *verb);
    int pushEvent(const char *event, struct json_object *param);

private:
    int tap_shortcut(afb_req_t request);
    int on_screen_message (afb_req_t request);
    int on_screen_reply (afb_req_t request);
    int showWindow(afb_req_t request);
    int hideWindow(afb_req_t request);
    int replyShowWindow(afb_req_t request);
    int subscribe(afb_req_t request);
    int unsubscribe(afb_req_t request);
    int showNotification(afb_req_t request);
    int showInformation(afb_req_t request);

    typedef int (HS_Client::*func_handler)(afb_req_t);
    static const std::unordered_map<std::string, func_handler> func_list;
    bool checkEvent(const char* event);
    bool isSupportEvent(const char* event);

private:
    std::string my_id;
    afb_event_t my_event;
    bool subscription = false;
    std::unordered_set<std::string> event_list;

};

#endif // HOMESCREEN_CLIENT_H