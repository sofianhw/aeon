/*
 Copyright 2016 Nervana Systems Inc.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this nds except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include <sstream>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <iostream>
#include <iterator>

#include "block_loader_nds_async.hpp"
#include "util.hpp"
#include "file_util.hpp"
#include "log.hpp"
#include "base64.hpp"
#include "json.hpp"
#include "interface.hpp"

using namespace std;
using namespace nervana;

block_loader_nds_async::block_loader_nds_async(const std::string& baseurl, const std::string& token, size_t collection_id, size_t block_size,
                                   size_t shard_count, size_t shard_index)
    : block_loader_source_async(*this)
    , m_baseurl(baseurl)
    , m_token(token)
    , m_collection_id(collection_id)
    , m_shard_count(shard_count)
    , m_shard_index(shard_index)
{
    for (int k = 0; k < 2; ++k)
    {
        // for (size_t j = 0; j < element_count(); ++j)
        // {
        //     m_containers[k].emplace_back();
        // }
    }
}

size_t write_data(void* ptr, size_t size, size_t nmemb, void* stream)
{
    stringstream& ss = *(stringstream*)stream;
    // callback used by curl.  writes data from ptr into the
    // stringstream passed in to `stream`.

    ss.write((const char*)ptr, size * nmemb);
    return size * nmemb;
}

nervana::variable_buffer_array* block_loader_nds_async::filler()
{
    variable_buffer_array* rc = get_pending_buffer();

    return rc;
}

void block_loader_nds_async::get(const string& url, stringstream& stream)
{
    // reuse curl connection across requests
    void* m_curl = curl_easy_init();

    // given a url, make an HTTP GET request and fill stream with
    // the body of the response

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &stream);
    // curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1L);

    // Perform the request, res will get the return code
    CURLcode res = curl_easy_perform(m_curl);

    // Check for errors
    long http_code = 0;
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200 || res != CURLE_OK)
    {
        stringstream ss;
        ss << "HTTP GET on \n'" << url << "' failed. ";
        ss << "status code: " << http_code;
        if (res != CURLE_OK)
        {
            ss << " curl return: " << curl_easy_strerror(res);
        }

        curl_easy_cleanup(m_curl);
        throw std::runtime_error(ss.str());
    }

    curl_easy_cleanup(m_curl);
}

const string block_loader_nds_async::load_block_url(uint32_t block_num)
{
    stringstream ss;
    ss << m_baseurl << "/macrobatch/?";
    ss << "macro_batch_index=" << block_num;
    ss << "&macro_batch_max_size=" << m_block_size;
    ss << "&collection_id=" << m_collection_id;
    ss << "&shard_count=" << m_shard_count;
    ss << "&shard_index=" << m_shard_index;
    ss << "&token=" << m_token;
    return ss.str();
}

const string block_loader_nds_async::metadata_url()
{
    stringstream ss;
    ss << m_baseurl << "/object_count/?";
    ss << "macro_batch_max_size=" << m_block_size;
    ss << "&collection_id=" << m_collection_id;
    ss << "&shard_count=" << m_shard_count;
    ss << "&shard_index=" << m_shard_index;
    ss << "&token=" << m_token;
    return ss.str();
}

void block_loader_nds_async::load_metadata()
{
    // fetch metadata and store in local attributes

    stringstream json_stream;
    get(metadata_url(), json_stream);
    string         json_str = json_stream.str();
    nlohmann::json metadata;
    try
    {
        metadata = nlohmann::json::parse(json_str);
    }
    catch (std::exception& ex)
    {
        stringstream ss;
        ss << "exception parsing metadata from nds ";
        ss << metadata_url() << " " << ex.what() << " ";
        ss << json_str;
        throw std::runtime_error(ss.str());
    }

    nervana::interface::config::parse_value(m_object_count, "record_count", metadata, nervana::interface::config::mode::REQUIRED);
    nervana::interface::config::parse_value(m_block_count, "macro_batch_per_shard", metadata,
                                            nervana::interface::config::mode::REQUIRED);
}

// size_t block_loader_nds_async::object_count() const
// {
//     return m_object_count;
// }
