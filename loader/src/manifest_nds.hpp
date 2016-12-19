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

#include <string>

#include "manifest.hpp"
#include "async_manager.hpp"
#include "buffer_batch.hpp"

namespace nervana
{
    class manifest_nds;
}

class nervana::manifest_nds : public nervana::async_manager_source<variable_buffer_array>,
                              public nervana::manifest
{
public:
    manifest_nds(const std::string& baseurl, const std::string& token, size_t collection_id, size_t block_size,
                 size_t shard_count = 1, size_t shard_index = 0);
    ~manifest_nds()
    {
    }

    variable_buffer_array* next() override;
    void reset() override
    {
    }

    size_t record_count() const override
    {
        return m_record_count;
    }

    size_t element_count() const override
    {
        return 2;
    }

    size_t block_count() const
    {
        return m_block_count;
    }

    std::vector<std::vector<char>> load_block(size_t block_index);

    std::string cache_id() override;

    // NDS manifests doesn't have versions since collections are immutable
    std::string version() override { return ""; }
    static bool is_likely_json(const std::string filename);

    void load_metadata();
    void get(const std::string& url, std::stringstream& stream);
    const std::string load_block_url(size_t block_index);
    const std::string metadata_url();

    std::string m_baseurl;
    std::string m_token;
    int         m_collection_id;
    size_t      m_block_size;
    const size_t             m_shard_count;
    const size_t             m_shard_index;
    size_t m_record_count;
    size_t m_block_count;

private:
    static size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream);
};
