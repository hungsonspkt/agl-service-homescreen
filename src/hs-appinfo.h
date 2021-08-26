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

#ifndef HOMESCREEN_APPINFO_H
#define HOMESCREEN_APPINFO_H

#include <string>
#include <mutex>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include "hs-helper.h"
#include "hs-proxy.h"


struct AppDetail {
    std::string name;
    std::string id;
    std::string detail; // detail json_object string
    bool periphery;

    std::string getProperty(std::string key) const;
};

class HS_AppInfo {
public:
    HS_AppInfo() = default;
    ~HS_AppInfo();
    HS_AppInfo(HS_AppInfo const &) = delete;
    HS_AppInfo &operator=(HS_AppInfo const &) = delete;
    HS_AppInfo(HS_AppInfo &&) = delete;
    HS_AppInfo &operator=(HS_AppInfo &&) = delete;

    static HS_AppInfo* instance(void);
    int init(afb_api_t api);
    int onEvent(afb_api_t api, const char *event, struct json_object *object);

    void getRunnables(struct json_object **object);
    std::string getAppProperty(const std::string appid, std::string key) const;
    std::string checkAppId(const std::string &appid);

private:
    int updateAppDetailList(afb_api_t api, struct json_object *object);
    void createAppDetailList(struct json_object *object);
    std::string parseAppDetail(struct json_object *object, AppDetail &info) const;
    void addAppDetail(struct json_object *object);
    void removeAppDetail(std::string appid);
    struct json_object* retrieveRunnables(struct json_object *obj_runnables, std::string id);
    void pushAppListChangedEvent(const char *oper, struct json_object *object);
    std::string id2appid(const std::string &id) const;
    bool isPeripheryApp(const char *appid) const;

    // applications can't display on launcher
    const std::vector<const char*> periphery_app_list {
        "launcher",
        "homescreen",
        "onscreenapp",
        "restriction"
    };

    typedef int (HS_AppInfo::*func_handler)(afb_api_t, struct json_object*);
    static const std::unordered_map<std::string, func_handler> concerned_event_list;

private:
    static HS_AppInfo* me;
    HS_AfmMainProxy* afmmain = nullptr;
    std::unordered_map<std::string, std::string> appid2name;
    std::unordered_map<std::string, std::string> name2appid;
    std::unordered_map<std::string, AppDetail> app_detail_list;
    std::mutex mtx;
};

#endif // HOMESCREEN_APPINFO_H