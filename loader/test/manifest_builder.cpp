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

#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <cstdio>
#include <unistd.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "manifest_builder.hpp"
#include "manifest_file.hpp"
#include "file_util.hpp"
#include "gen_image.hpp"
#include "base64.hpp"

using namespace std;
using namespace nervana;

stringstream& manifest_builder::create()
{
    m_stream.str("");
    if (m_sizes.size() > 0)
    {
        // all exements are text elements
        m_stream << manifest_file::get_metadata_char();
        for (size_t i=0; i<m_sizes.size(); i++)
        {
            if (i>0)
            {
                m_stream << manifest_file::get_delimiter();
            }
            m_stream << manifest_file::get_string_type_id();
        }
        m_stream << endl;

        // now add the records
        for (size_t record_number=0; record_number < m_record_count; record_number++)
        {
            for (size_t element_number=0; element_number<m_sizes.size(); element_number++)
            {
                if (element_number>0)
                {
                    m_stream << manifest_file::get_delimiter();
                }
                m_stream << record_number << ":" << element_number;
            }
            m_stream << endl;
        }
    }
    else if (m_image_width > 0 && m_image_height > 0)
    {
        // first element image, second element label
        size_t rows = 8;
        size_t cols = 8;
        m_stream << manifest_file::get_metadata_char() << manifest_file::get_binary_type_id();
        m_stream << manifest_file::get_delimiter() << manifest_file::get_string_type_id();
        m_stream << endl;
        for (size_t record_number=0; record_number < m_record_count; record_number++)
        {
            cv::Mat mat = embedded_id_image::generate_image(rows, cols, record_number);
            vector<uint8_t> result;
            cv::imencode(".png", mat, result);
            vector<char> image_data = base64::encode((const char*)result.data(), result.size());
            m_stream << vector2string(image_data);
            m_stream << manifest_file::get_delimiter();
            m_stream << record_number;
            m_stream << endl;
        }
    }
    else
    {
        throw invalid_argument("must set either sizes or image dimensions");
    }

    return m_stream;
}

manifest_builder& manifest_builder::record_count(size_t value)
{
    m_record_count = value;
    return *this;
}

manifest_builder& manifest_builder::sizes(const std::vector<size_t>& sizes)
{
    m_sizes = sizes;
    return *this;
}

manifest_builder& manifest_builder::image_width(size_t value)
{
    m_image_width = value;
    return *this;
}

manifest_builder& manifest_builder::image_height(size_t value)
{
    m_image_height = value;
    return *this;
}


//manifest_builder::manifest_builder(size_t record_count, std::vector<size_t> sizes)
//{
//    manifest_name = tmp_manifest_file(record_count, sizes);
//}

//manifest_builder::manifest_builder(size_t record_count, int height, int width)
//{
//    manifest_name = image_manifest(record_count, height, width);
//}

//manifest_builder::manifest_builder()
//{
//}

//manifest_builder::~manifest_builder()
//{
//    remove_files();
//}

//std::string manifest_builder::get_manifest_name()
//{
//    return manifest_name;
//}

//void manifest_builder::remove_files()
//{
//    for (auto it : tmp_filenames)
//    {
//        remove(it.c_str());
//    }
//}

//string manifest_builder::tmp_filename(const string& extension)
//{
//    string tmpname = file_util::tmp_filename(extension);
//    tmp_filenames.push_back(tmpname);
//    return tmpname;
//}

//string manifest_builder::image_manifest(size_t record_count, int height, int width)
//{
//    string   tmpname = tmp_filename();
//    ofstream f_manifest(tmpname);

//    for (uint32_t i = 0; i < record_count; ++i)
//    {
//        cv::Mat mat = embedded_id_image::generate_image(height, width, i);
////        cv::Mat mat{height, width, CV_8UC3};
////        mat = cv::Scalar(0,0,0);
//        string image_path = tmp_filename("_" + std::to_string(i) + ".png");
//        string target_path = tmp_filename();
//        f_manifest << image_path << manifest_file::get_delimiter() << target_path << '\n';
//        cv::imwrite(image_path, mat);
//        {
//            ofstream f(target_path);
//            int value = 0;
//            f.write((const char*)&value, sizeof(value));
//        }
//    }

//    f_manifest.close();

//    return tmpname;
//}

//string manifest_builder::tmp_manifest_file(size_t record_count, vector<size_t> sizes)
//{
//    string   tmpname = tmp_filename();
//    ofstream f(tmpname);

//    for (uint32_t i = 0; i < record_count; ++i)
//    {
//        // stick a unique uint32_t into each file
//        for (uint32_t j = 0; j < sizes.size(); ++j)
//        {
//            if (j != 0)
//            {
//                f << manifest_file::get_delimiter();
//            }

//            f << tmp_file_repeating(sizes[j], (i * sizes.size()) + j);
//        }
//        f << '\n';
//    }

//    f.close();

//    return tmpname;
//}

//string manifest_builder::tmp_file_repeating(size_t size, uint32_t x)
//{
//    // create a temp file of `size` bytes filled with uint32_t x
//    string   tmpname = tmp_filename();
//    ofstream f(tmpname, ios::binary);

//    uint32_t repeats = size / sizeof(x);
//    for (uint32_t i = 0; i < repeats; ++i)
//    {
//        f.write(reinterpret_cast<const char*>(&x), sizeof(x));
//    }

//    f.close();

//    return tmpname;
//}

//std::string manifest_builder::tmp_manifest_file_with_invalid_filename()
//{
//    string   tmpname = tmp_filename();
//    ofstream f(tmpname);

//    for (uint32_t i = 0; i < 10; ++i)
//    {
//        f << tmp_filename() + ".this_file_shouldnt_exist" << manifest_file::get_delimiter();
//        f << tmp_filename() + ".this_file_shouldnt_exist" << endl;
//    }

//    f.close();
//    return tmpname;
//}
