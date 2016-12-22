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

#include "manifest_file.hpp"
#include "buffer_batch.hpp"
#include "block_loader_source_async.hpp"

/* block_loader_file
 *
 * Loads blocks of files from a Manifest into a BufferPair.
 *
 */

namespace nervana
{
    class block_loader_file_async;
}

class nervana::block_loader_file_async : public block_loader_source_async
{
public:
    block_loader_file_async(manifest_file* mfst, size_t block_size);
    virtual ~block_loader_file_async()
    {
        finalize();
    }

    encoded_record_list* filler() override;
    void reset() override;

    size_t block_count() const override
    {
        return m_manifest.block_count();
    }

    size_t record_count() const override
    {
        return m_manifest.record_count();
    }

    size_t block_size() const override
    {
        return 1;
    }

    size_t elements_per_record() const override
    {
        return m_elements_per_record;
    }

    source_uid_t get_uid() const override
    {
        return m_manifest.get_crc();
    }

private:
    size_t        m_block_size;
    size_t        m_block_count;
    size_t        m_record_count;
    size_t        m_elements_per_record;
    manifest_file& m_manifest;
};
