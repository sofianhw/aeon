/*
 Copyright 2016 Nervana Systems Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#pragma once

#include "web_server.hpp"

class web_app
{
public:
    web_app();
    ~web_app();
    static void start();

    void home_page(web::page& p);
    void forms_demo(web::page& p);
    void forms_response(web::page& p);
    void page_404(web::page& p);
    void process_page_request(web::page& p, const std::string& url);

private:
    web::server web_server;
};
