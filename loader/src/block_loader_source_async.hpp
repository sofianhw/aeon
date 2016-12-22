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

#include <vector>
#include <string>

#include "async_manager.hpp"
#include "buffer_batch.hpp"

namespace nervana
{
    class block_loader_source_async;

    typedef uint32_t source_uid_t;
}

class nervana::block_loader_source_async
    : public async_manager<std::vector<std::vector<std::string>>, encoded_record_list>
{
public:
    block_loader_source_async(async_manager_source<std::vector<std::vector<std::string>>>* source)
        : async_manager<std::vector<std::vector<std::string>>, encoded_record_list>(source)
    {
        // Make the container pair?  Currently letting child handle it in filler()
    }

    virtual size_t record_count() const = 0;
    virtual size_t block_size() const = 0;
    virtual size_t block_count() const = 0;
    virtual size_t elements_per_record() const = 0;
    virtual source_uid_t get_uid() const = 0;
};
