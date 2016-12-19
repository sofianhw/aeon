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
#include "async_manager.hpp"
#include "block_manager_async.hpp"
#include "buffer_batch.hpp"
#include "util.hpp"

/* block_loader_file
 *
 * Loads blocks of files from a Manifest into a BufferPair.
 *
 */

namespace nervana
{
    class batch_iterator_async;
}

class nervana::batch_iterator_async
    : public async_manager<encoded_record_list, encoded_record_list>
{
public:
    batch_iterator_async(block_manager_async*, size_t batch_size);
    virtual ~batch_iterator_async() { finalize(); }
    virtual size_t record_count() const override { return m_batch_size; }
    virtual encoded_record_list* filler() override;

    virtual void initialize() override
    {
        async_manager<encoded_record_list, encoded_record_list>::initialize();
        m_input_ptr = nullptr;
    }

private:
    void move_src_to_dst(encoded_record_list* src_array_ptr, encoded_record_list* dst_array_ptr, size_t count);

    size_t                 m_batch_size;
    size_t                 m_element_count;
    encoded_record_list* m_input_ptr{nullptr};
};
