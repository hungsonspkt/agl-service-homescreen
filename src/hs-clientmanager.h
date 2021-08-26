/*
 * Copyright (c) 2018 TOYOTA MOTOR CORPORATION
 * Copyright (C) 2020 Konsulko Group
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

#ifndef HOMESCREEN_CLIENTMANAGER_H
#define HOMESCREEN_CLIENTMANAGER_H

#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>
#include "hs-helper.h"
#include "hs-client.h"

struct HS_ClientCtxt {
    std::string id;
    HS_ClientCtxt(const std::string &appid)
    {
        id = appid;
    }
};


class HS_ClientManager {
public:
    HS_ClientManager();
    ~HS_ClientManager() = default;
    HS_ClientManager(HS_ClientManager const &) = delete;
    HS_ClientManager &operator=(HS_ClientManager const &) = delete;
    HS_ClientManager(HS_ClientManager &&) = delete;
    HS_ClientManager &operator=(HS_ClientManager &&) = delete;

    static HS_ClientManager* instance(void);
    int init(void);
    int handleRequest(afb_req_t request, const char *verb, const char *appid = nullptr);
    int pushEvent(const char *event, struct json_object *param, std::string appid = "");
    void removeClientCtxt(void *data);  // don't use, internal only

    HS_ClientCtxt* createClientCtxt(afb_req_t req, std::string appid);
    HS_Client* addClient(afb_req_t req, std::string appid);
    void removeClient(std::string appid);

private:
    static HS_ClientManager* me;
    std::unordered_map<std::string, HS_Client*> client_list;
    std::unordered_map<std::string, HS_ClientCtxt*> appid2ctxt;
    std::mutex mtx;
};

#endif // HOMESCREEN_CLIENTMANAGER_H
