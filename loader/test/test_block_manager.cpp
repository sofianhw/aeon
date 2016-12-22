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

#include <vector>

#include "gtest/gtest.h"
#include "file_util.hpp"
#include "log.hpp"
#include "manifest_builder.hpp"
#include "manifest_file.hpp"
#include "block_loader_file_async.hpp"
#include "block_loader_source_async.hpp"
#include "block.hpp"

#define private public

#include "block_manager_async.hpp"

using namespace std;
using namespace nervana;

TEST(block_manager, block_list)
{
    {
        vector<block_info> seq = generate_block_list(1003, 335);
        ASSERT_EQ(3, seq.size());

        EXPECT_EQ(0, seq[0].start());
        EXPECT_EQ(335, seq[0].count());

        EXPECT_EQ(335, seq[1].start());
        EXPECT_EQ(335, seq[1].count());

        EXPECT_EQ(670, seq[2].start());
        EXPECT_EQ(333, seq[2].count());
    }
    {
        vector<block_info> seq = generate_block_list(20, 5000);
        ASSERT_EQ(1, seq.size());

        EXPECT_EQ(0, seq[0].start());
        EXPECT_EQ(20, seq[0].count());
    }
}

TEST(block_manager, cache_complete)
{
    string cache_root = file_util::make_temp_directory();

    EXPECT_FALSE(block_manager_async::check_if_complete(cache_root));
    block_manager_async::mark_cache_complete(cache_root);
    EXPECT_TRUE(block_manager_async::check_if_complete(cache_root));

    file_util::remove_directory(cache_root);
}

TEST(block_manager, cache_ownership)
{
    string cache_root = file_util::make_temp_directory();

    int lock;
    EXPECT_TRUE(block_manager_async::take_ownership(cache_root, lock));

    int lock2;
    EXPECT_FALSE(block_manager_async::take_ownership(cache_root, lock2));

    block_manager_async::release_ownership(cache_root, lock);

    EXPECT_TRUE(block_manager_async::take_ownership(cache_root, lock));
    block_manager_async::release_ownership(cache_root, lock);

    file_util::remove_directory(cache_root);
}

TEST(block_manager, cache_busy)
{
    string cache_root = file_util::make_temp_directory();

    manifest_builder mb;

    size_t record_count    = 10;
    size_t block_size      = 4;
    size_t object_size     = 16;
    size_t target_size     = 16;

    stringstream& manifest_stream = mb.sizes({object_size, target_size}).record_count(record_count).create();
    manifest_file manifest(manifest_stream, false);

    block_loader_file_async file_reader(&manifest, block_size);
    string cache_name = block_manager_async::create_cache_name(file_reader.get_uid());
    auto cache_dir = file_util::path_join(cache_root, cache_name);

    int lock;
    file_util::make_directory(cache_dir);
    EXPECT_TRUE(block_manager_async::take_ownership(cache_dir, lock));

    EXPECT_THROW(block_manager_async(&file_reader, block_size, cache_root, false), runtime_error);

    block_manager_async::release_ownership(cache_root, lock);

    file_util::remove_directory(cache_root);
}

TEST(block_manager, build_cache)
{
    string cache_root = file_util::make_temp_directory();

    manifest_builder mb;

    size_t record_count    = 12;
    size_t block_size      = 4;
    size_t object_size     = 16;
    size_t target_size     = 16;
    size_t block_count     = record_count / block_size;
    ASSERT_EQ(0, record_count % block_size);
    ASSERT_EQ(0, object_size % sizeof(uint32_t));
    ASSERT_EQ(0, target_size % sizeof(uint32_t));

    stringstream& manifest_stream = mb.sizes({object_size, target_size}).record_count(record_count).create();
    manifest_file manifest(manifest_stream, false, "", 1.0, block_size);

    block_loader_file_async file_reader(&manifest, block_size);
    string cache_name = block_manager_async::create_cache_name(file_reader.get_uid());
    auto cache_dir = file_util::path_join(cache_root, cache_name);

    block_manager_async manager(&file_reader, block_size, cache_root, false);

    size_t record_number = 0;
    for (size_t i=0; i<block_count*2; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(block_size, buffer->size());

        for (size_t i=0; i<block_size; i++)
        {
            const encoded_record& record = buffer->record(i);
            for (size_t element_number=0; element_number<record.size(); element_number++)
            {
                stringstream ss;
                ss << record_number << ":" << element_number;
                string expected = ss.str();
                string element = vector2string(record.element(element_number));
                ASSERT_STREQ(expected.c_str(), element.c_str());
            }
            record_number = (record_number + 1) % record_count;
        }
    }

    // check that the cache files exist
    string cache_complete = block_manager_async::m_cache_complete_filename;
    string cache_complete_path = file_util::path_join(cache_dir, cache_complete);
    EXPECT_TRUE(file_util::exists(cache_complete_path));
    for (size_t block_number=0; block_number<block_count; block_number++)
    {
        string cache_block_name = manager.create_cache_block_name(block_number);
        string cache_block_path = file_util::path_join(cache_dir, cache_block_name);
        EXPECT_TRUE(file_util::exists(cache_block_path));
    }

    EXPECT_EQ(block_count, manager.m_cache_hit);
    EXPECT_EQ(block_count, manager.m_cache_miss);

    file_util::remove_directory(cache_root);
}

TEST(block_manager, reuse_cache)
{
    string cache_root = file_util::make_temp_directory();

    manifest_builder mb;

    size_t record_count    = 12;
    size_t block_size      = 4;
    size_t object_size     = 16;
    size_t target_size     = 16;
    size_t block_count     = record_count / block_size;
    ASSERT_EQ(0, record_count % block_size);
    ASSERT_EQ(0, object_size % sizeof(uint32_t));
    ASSERT_EQ(0, target_size % sizeof(uint32_t));

    stringstream& manifest_stream = mb.sizes({object_size, target_size}).record_count(record_count).create();
    manifest_file manifest(manifest_stream, false, "", 1.0, block_size);

    // first build the cache
    {
        block_loader_file_async file_reader(&manifest, block_size);

        block_manager_async manager(&file_reader, block_size, cache_root, false);

        size_t record_number = 0;
        for (size_t i=0; i<block_count; i++)
        {
            encoded_record_list* buffer = manager.next();
            ASSERT_NE(nullptr, buffer);
            ASSERT_EQ(4, buffer->size());

            for (size_t i=0; i<block_size; i++)
            {
                const encoded_record& record = buffer->record(i);
                for (size_t element_number=0; element_number<record.size(); element_number++)
                {
                    stringstream ss;
                    ss << record_number << ":" << element_number;
                    string expected = ss.str();
                    string element = vector2string(record.element(element_number));
                    ASSERT_STREQ(expected.c_str(), element.c_str());
                }
                record_number = (record_number + 1) % record_count;
            }
        }
        ASSERT_EQ(0, manager.m_cache_hit);
        ASSERT_EQ(block_count, manager.m_cache_miss);
    }

    // now read data with new reader, same manifest
    {
        block_loader_file_async file_reader(&manifest, block_size);

        block_manager_async manager(&file_reader, block_size, cache_root, false);

        size_t record_number = 0;
        for (size_t i=0; i<block_count; i++)
        {
            encoded_record_list* buffer = manager.next();
            ASSERT_NE(nullptr, buffer);
            ASSERT_EQ(4, buffer->size());

            for (size_t i=0; i<block_size; i++)
            {
                const encoded_record& record = buffer->record(i);
                for (size_t element_number=0; element_number<record.size(); element_number++)
                {
                    stringstream ss;
                    ss << record_number << ":" << element_number;
                    string expected = ss.str();
                    string element = vector2string(record.element(element_number));
                    ASSERT_STREQ(expected.c_str(), element.c_str());
                }
                record_number = (record_number + 1) % record_count;
            }
        }
        EXPECT_EQ(block_count, manager.m_cache_hit);
        EXPECT_EQ(0, manager.m_cache_miss);
    }

    file_util::remove_directory(cache_root);
}

TEST(block_manager, no_shuffle_cache)
{
    manifest_builder mb;

    string cache_root = file_util::make_temp_directory();
    size_t record_count    = 12;
    size_t block_size      = 4;
    size_t object_size     = 16;
    size_t target_size     = 16;
    size_t block_count     = record_count / block_size;
    bool enable_shuffle = false;
    ASSERT_EQ(0, record_count % block_size);
    ASSERT_EQ(0, object_size % sizeof(uint32_t));
    ASSERT_EQ(0, target_size % sizeof(uint32_t));

    vector<size_t> sorted_record_list(record_count);
    iota(sorted_record_list.begin(), sorted_record_list.end(), 0);

    stringstream& manifest_stream = mb.sizes({object_size, target_size}).record_count(record_count).create();
    manifest_file manifest(manifest_stream, enable_shuffle, "", 1.0, block_size);
    block_loader_file_async file_reader(&manifest, block_size);
    block_manager_async manager(&file_reader, block_size, cache_root, enable_shuffle);

    vector<size_t> first_pass;
    vector<size_t> second_pass;
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            first_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));
    EXPECT_TRUE(equal(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));

    // second read should be shuffled
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            second_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_TRUE(equal(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_TRUE(equal(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));

    file_util::remove_directory(cache_root);
}

TEST(block_manager, no_shuffle_no_cache)
{
    manifest_builder mb;

    string cache_root = "";
    size_t record_count    = 12;
    size_t block_size      = 4;
    size_t object_size     = 16;
    size_t target_size     = 16;
    size_t block_count     = record_count / block_size;
    bool enable_shuffle = false;
    ASSERT_EQ(0, record_count % block_size);
    ASSERT_EQ(0, object_size % sizeof(uint32_t));
    ASSERT_EQ(0, target_size % sizeof(uint32_t));

    vector<size_t> sorted_record_list(record_count);
    iota(sorted_record_list.begin(), sorted_record_list.end(), 0);

    stringstream& manifest_stream = mb.sizes({object_size, target_size}).record_count(record_count).create();
    manifest_file manifest(manifest_stream, enable_shuffle, "", 1.0, block_size);
    block_loader_file_async file_reader(&manifest, block_size);
    block_manager_async manager(&file_reader, block_size, cache_root, enable_shuffle);

    vector<size_t> first_pass;
    vector<size_t> second_pass;
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            first_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));
    EXPECT_TRUE(equal(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));

    // second read should be shuffled
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            second_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_TRUE(equal(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_TRUE(equal(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));
}

TEST(block_manager, shuffle_cache)
{
    manifest_builder mb;

    string cache_root = file_util::make_temp_directory();
    size_t record_count    = 12;
    size_t block_size      = 4;
    size_t object_size     = 16;
    size_t target_size     = 16;
    size_t block_count     = record_count / block_size;
    bool enable_shuffle = true;
    ASSERT_EQ(0, record_count % block_size);
    ASSERT_EQ(0, object_size % sizeof(uint32_t));
    ASSERT_EQ(0, target_size % sizeof(uint32_t));

    vector<size_t> sorted_record_list(record_count);
    iota(sorted_record_list.begin(), sorted_record_list.end(), 0);

    stringstream& manifest_stream = mb.sizes({object_size, target_size}).record_count(record_count).create();
    manifest_file manifest(manifest_stream, enable_shuffle, "", 1.0, block_size);
    block_loader_file_async file_reader(&manifest, block_size);
    block_manager_async manager(&file_reader, block_size, cache_root, enable_shuffle);

    vector<size_t> first_pass;
    vector<size_t> second_pass;
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            first_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));
    EXPECT_FALSE(equal(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));

    // second read should be shuffled
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            second_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_FALSE(equal(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_FALSE(equal(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));

    file_util::remove_directory(cache_root);
}

TEST(block_manager, shuffle_no_cache)
{
    manifest_builder mb;

    string cache_root = "";
    size_t record_count    = 12;
    size_t block_size      = 4;
    size_t object_size     = 16;
    size_t target_size     = 16;
    size_t block_count     = record_count / block_size;
    bool enable_shuffle = true;
    ASSERT_EQ(0, record_count % block_size);
    ASSERT_EQ(0, object_size % sizeof(uint32_t));
    ASSERT_EQ(0, target_size % sizeof(uint32_t));

    vector<size_t> sorted_record_list(record_count);
    iota(sorted_record_list.begin(), sorted_record_list.end(), 0);

    stringstream& manifest_stream = mb.sizes({object_size, target_size}).record_count(record_count).create();
    manifest_file manifest(manifest_stream, enable_shuffle, "", 1.0, block_size);
    block_loader_file_async file_reader(&manifest, block_size);
    block_manager_async manager(&file_reader, block_size, cache_root, enable_shuffle);

    vector<size_t> first_pass;
    vector<size_t> second_pass;
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            first_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));
    EXPECT_FALSE(equal(first_pass.begin(), first_pass.end(), sorted_record_list.begin()));

    // second read should be shuffled
    for (size_t i=0; i<block_count; i++)
    {
        encoded_record_list* buffer = manager.next();
        ASSERT_NE(nullptr, buffer);
        ASSERT_EQ(4, buffer->size());

        for (size_t record=0; record<block_size; record++)
        {
            string data0 = vector2string(buffer->record(record).element(0));
            size_t value = stod(split(data0, ':')[0]);
            second_pass.push_back(value);
        }
    }
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));
    EXPECT_TRUE(is_permutation(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_FALSE(equal(second_pass.begin(), second_pass.end(), first_pass.begin()));
    EXPECT_FALSE(equal(second_pass.begin(), second_pass.end(), sorted_record_list.begin()));
}
