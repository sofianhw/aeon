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

#include <memory>
#include "provider_interface.hpp"

namespace nervana
{
    class provider_factory;
}

class nervana::provider_factory
{
public:
    virtual ~provider_factory() {}
public:
    static std::shared_ptr<nervana::provider_interface> create(nlohmann::json configJs);
    static std::shared_ptr<nervana::provider_interface>
        clone(const std::shared_ptr<nervana::provider_interface>& r);
};
