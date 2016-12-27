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
#include <string>
#include <sstream>
#include <random>
#include <future>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "gtest/gtest.h"
#include "util.hpp"
#include "wav_data.hpp"
#include "cap_mjpeg_decoder.hpp"
#include "image.hpp"

#define private public

#include "etl_audio.hpp"
#include "etl_boundingbox.hpp"
#include "etl_char_map.hpp"
#include "etl_image.hpp"
#include "etl_label.hpp"
#include "etl_label_map.hpp"
#include "etl_localization.hpp"
#include "etl_multicrop.hpp"
#include "etl_pixel_mask.hpp"
#include "etl_video.hpp"
#include "loader.hpp"

using namespace std;
using namespace nervana;

TEST(util, unpack_le)
{
    {
        char data[] = {1, 0, 0, 0};
        int  actual = unpack<int>(data);
        EXPECT_EQ(0x00000001, actual);
    }
    {
        char data[] = {0, 1, 0, 0};
        int  actual = unpack<int>(data);
        EXPECT_EQ(0x00000100, actual);
    }
    {
        char data[] = {0, 0, 0, 1};
        int  actual = unpack<int>(data);
        EXPECT_EQ(0x01000000, actual);
    }
    // {
    //     char data[] = {0,0,0,1};
    //     int actual = unpack<int>(data,0);
    //     EXPECT_EQ(0,actual);
    // }
    {
        char data[] = {0, 0, 0, 1};
        int  actual = unpack<int>(data, 1);
        EXPECT_EQ(0x00010000, actual);
    }
    {
        char data[] = {(char)0x80, 0, 0, 0};
        int  actual = unpack<int>(data);
        EXPECT_EQ(128, actual);
    }

    {
        float a = 25.234;
        char  data[4];
        pack<float>(data, a);

        float b = unpack<float>(data);
        EXPECT_FLOAT_EQ(a, b);
    }
}

TEST(util, unpack_be)
{
    {
        char data[] = {0, 0, 0, 1};
        int  actual = unpack<int>(data, 0, endian::BIG);
        EXPECT_EQ(0x00000001, actual);
    }
    {
        char data[] = {0, 0, 1, 0};
        int  actual = unpack<int>(data, 0, endian::BIG);
        EXPECT_EQ(0x00000100, actual);
    }
    {
        char data[] = {1, 0, 0, 0};
        int  actual = unpack<int>(data, 0, endian::BIG);
        EXPECT_EQ(0x01000000, actual);
    }
    {
        char data[] = {1, 0, 0, 0};
        int  actual = unpack<int>(data, 1, endian::BIG);
        EXPECT_EQ(0, actual);
    }
}

TEST(util, pack_le)
{
    {
        char actual[]   = {0, 0, 0, 0};
        char expected[] = {1, 0, 0, 0};
        pack<int>(actual, 1);
        EXPECT_EQ(*(unsigned int*)expected, *(unsigned int*)actual);
    }
    {
        char actual[]   = {0, 0, 0, 0};
        char expected[] = {0, 1, 0, 0};
        pack<int>(actual, 0x00000100);
        EXPECT_EQ(*(unsigned int*)expected, *(unsigned int*)actual);
    }
    {
        char actual[]   = {0, 0, 0, 0};
        char expected[] = {0, 0, 0, 1};
        pack<int>(actual, 0x01000000);
        EXPECT_EQ(*(unsigned int*)expected, *(unsigned int*)actual);
    }

    //    {
    //        char actual[] = {0,0,0,0};
    //        char expected[] = {0,0,0,1};
    //        pack_le<int>(actual,0,3);
    //        EXPECT_EQ(expected,actual);
    //    }
    //    {
    //        char actual[] = {0,0,0,0};
    //        char expected[] = {0,0,0,1};
    //        pack_le<int>(actual,0x00010000,1,3);
    //        EXPECT_EQ(expected,actual);
    //    }
}

TEST(avi, video_file)
{
    const string                  filename  = CURDIR "/test_data/bb8.avi";
    shared_ptr<MotionJpegCapture> mjdecoder = make_shared<MotionJpegCapture>(filename);
    ASSERT_TRUE(mjdecoder->isOpened());
    cv::Mat image;
    int     image_number = 0;
    while (mjdecoder->grabFrame() && mjdecoder->retrieveFrame(0, image))
    {
        ASSERT_NE(0, image.size().area());
        //        string output_name = "mjpeg_frame_"+to_string(image_number)+".jpg";
        //        cv::imwrite(output_name,image);
        image_number++;
    }
    EXPECT_EQ(6, image_number);
}

TEST(avi, video_buffer)
{
    const string filename = CURDIR "/test_data/bb8.avi";
    ifstream     in(filename, ios_base::binary);
    ASSERT_TRUE(in);
    in.seekg(0, in.end);
    size_t size = in.tellg();
    in.seekg(0, in.beg);
    vector<char> data(size);
    data.assign(istreambuf_iterator<char>(in), istreambuf_iterator<char>());

    shared_ptr<MotionJpegCapture> mjdecoder =
        make_shared<MotionJpegCapture>(data.data(), data.size());
    ASSERT_TRUE(mjdecoder->isOpened());
    cv::Mat image;
    int     image_number = 0;
    while (mjdecoder->grabFrame() && mjdecoder->retrieveFrame(0, image))
    {
        ASSERT_NE(0, image.size().area());
        //        string output_name = "mjpeg_frame_"+to_string(image_number)+".jpg";
        //        cv::imwrite(output_name,image);
        image_number++;
    }
    EXPECT_EQ(6, image_number);
}

TEST(util, memstream)
{
    string          data = "abcdefghijklmnopqrstuvwxyz";
    memstream<char> ms((char*)data.data(), data.size());
    istream         is(&ms);
    char            buf[10];

    EXPECT_EQ(0, is.tellg());
    is.seekg(0, is.end);
    EXPECT_EQ(26, is.tellg());
    is.seekg(10, is.end);
    EXPECT_EQ(16, is.tellg());
    is.seekg(3, is.cur);
    EXPECT_EQ(19, is.tellg());
    is.seekg(3, is.beg);
    EXPECT_EQ(3, is.tellg());
    is.read(buf, 2);
    EXPECT_EQ('d', buf[0]);
    EXPECT_EQ('e', buf[1]);
    is.read(buf, 2);
    EXPECT_EQ('f', buf[0]);
    EXPECT_EQ('g', buf[1]);

    EXPECT_EQ(true, is.good());
    is.seekg(0, is.end);
    is.read(buf, 2); // read past end
    EXPECT_EQ(false, is.good());

    // test stream reset
}

TEST(util, mixchannels)
{
    int rows = 10;
    int cols = 10;
    {
        int             channels = 1;
        vector<cv::Mat> source;
        vector<cv::Mat> target;
        vector<int>     from_to;
        source.emplace_back(rows, cols, CV_8UC(channels));
        uint8_t* p     = source.back().ptr<uint8_t>();
        int      index = 0;
        for (int i = 0; i < rows * cols; i++)
        {
            for (int j = 0; j < channels; j++)
            {
                p[index++] = i;
            }
        }
        for (int i = 0; i < channels; i++)
        {
            target.emplace_back(rows, cols, CV_8UC1);
            from_to.push_back(i);
            from_to.push_back(i);
        }
        image::convert_mix_channels(source, target, from_to);

        p     = target[0].ptr<uint8_t>();
        index = 0;
        for (int i = 0; i < rows * cols; i++)
        {
            for (int j = 0; j < channels; j++)
            {
                EXPECT_EQ(i, p[index++]);
            }
        }
    }

    {
        int             channels = 1;
        vector<cv::Mat> source;
        vector<cv::Mat> target;
        vector<int>     from_to;
        source.emplace_back(rows, cols, CV_8UC(channels));
        uint8_t* p     = source.back().ptr<uint8_t>();
        int      index = 0;
        for (int i = 0; i < rows * cols; i++)
        {
            for (int j = 0; j < channels; j++)
            {
                p[index++] = i;
            }
        }
        for (int i = 0; i < channels; i++)
        {
            target.emplace_back(rows, cols, CV_32SC1);
            from_to.push_back(i);
            from_to.push_back(i);
        }
        image::convert_mix_channels(source, target, from_to);

        int* p1 = target[0].ptr<int32_t>();
        index   = 0;
        for (int i = 0; i < rows * cols; i++)
        {
            for (int j = 0; j < channels; j++)
            {
                EXPECT_EQ(i, p1[index++]);
            }
        }
    }
}

TEST(util, distance)
{
    std::string s1 = "aspect_ratio";
    std::string s2 = "crops_per_scale";

    EXPECT_EQ(12, LevenshteinDistance(s1, s2));
}

void dump_config_info(ostream& f, shared_ptr<nervana::interface::config_info_interface> x)
{
    f << "   " << x->name() << "," << x->type() << "," << (x->required() ? "REQUIRED" : "OPTIONAL");
    f << "," << (x->required() ? "" : x->get_default_value());
    f << "\n";
}

#define DUMP_CONFIG(arg)                                                                           \
    {                                                                                              \
        arg::config cfg;                                                                           \
        f << #arg << ":\n";                                                                        \
        for (auto x : cfg.config_list)                                                             \
            dump_config_info(f, x);                                                                \
    }

TEST(util, param_dump)
{
    ofstream f("config_args.txt", ios::trunc);
    DUMP_CONFIG(audio);
    DUMP_CONFIG(boundingbox);
    DUMP_CONFIG(char_map);
    DUMP_CONFIG(image);
    DUMP_CONFIG(label);
    DUMP_CONFIG(label_map);
    DUMP_CONFIG(localization);
    DUMP_CONFIG(multicrop);
    DUMP_CONFIG(video);
    {
        loader_config cfg;
        f << "loader_config"
          << ":\n";
        for (auto x : cfg.config_list)
        {
            dump_config_info(f, x);
        }
    }
}

TEST(util, split)
{
    {
        string s1 = "this,is,a,test";
        auto   r1 = split(s1, ',');
        ASSERT_EQ(4, r1.size());
        EXPECT_STRCASEEQ("this", r1[0].c_str());
        EXPECT_STRCASEEQ("is", r1[1].c_str());
        EXPECT_STRCASEEQ("a", r1[2].c_str());
        EXPECT_STRCASEEQ("test", r1[3].c_str());
    }

    {
        string s1 = "this,is,a,test,";
        auto   r1 = split(s1, ',');
        ASSERT_EQ(5, r1.size());
        EXPECT_STRCASEEQ("this", r1[0].c_str());
        EXPECT_STRCASEEQ("is", r1[1].c_str());
        EXPECT_STRCASEEQ("a", r1[2].c_str());
        EXPECT_STRCASEEQ("test", r1[3].c_str());
        EXPECT_STRCASEEQ("", r1[4].c_str());
    }

    {
        string s1 = ",this,is,a,test";
        auto   r1 = split(s1, ',');
        ASSERT_EQ(5, r1.size());
        EXPECT_STRCASEEQ("", r1[0].c_str());
        EXPECT_STRCASEEQ("this", r1[1].c_str());
        EXPECT_STRCASEEQ("is", r1[2].c_str());
        EXPECT_STRCASEEQ("a", r1[3].c_str());
        EXPECT_STRCASEEQ("test", r1[4].c_str());
    }

    {
        string s1 = "this,,is,a,test";
        auto   r1 = split(s1, ',');
        ASSERT_EQ(5, r1.size());
        EXPECT_STRCASEEQ("this", r1[0].c_str());
        EXPECT_STRCASEEQ("", r1[1].c_str());
        EXPECT_STRCASEEQ("is", r1[2].c_str());
        EXPECT_STRCASEEQ("a", r1[3].c_str());
        EXPECT_STRCASEEQ("test", r1[4].c_str());
    }

    {
        string s1 = "this";
        auto   r1 = split(s1, ',');
        ASSERT_EQ(1, r1.size());
        EXPECT_STRCASEEQ("this", r1[0].c_str());
    }

    {
        string s1 = "";
        auto   r1 = split(s1, ',');
        ASSERT_EQ(1, r1.size());
        EXPECT_STRCASEEQ("", r1[0].c_str());
    }
}

TEST(util, unbiased_round)
{
    EXPECT_EQ(2, nervana::unbiased_round(1.5));
    EXPECT_EQ(2, nervana::unbiased_round(2.5));
    EXPECT_EQ(0, nervana::unbiased_round(-0.5));
    EXPECT_EQ(0, nervana::unbiased_round(0.5));
    EXPECT_EQ(622, nervana::unbiased_round(622.5));
    EXPECT_EQ(622, nervana::unbiased_round(621.5));
    EXPECT_EQ(1, nervana::unbiased_round(1.1));
    EXPECT_EQ(2, nervana::unbiased_round(2.1));
    EXPECT_EQ(901, nervana::unbiased_round(900.9));
    EXPECT_EQ(-1, nervana::unbiased_round(-1.1));
    EXPECT_EQ(-2, nervana::unbiased_round(-2.1));
    EXPECT_EQ(-2, nervana::unbiased_round(-2.5));
    EXPECT_EQ(-2, nervana::unbiased_round(-1.5));
}

TEST(DISABLED_util, dump)
{
    string text = "this is a text string used to test the dump function.";

    dump(cout, text.data(), text.size());
}

// static void test_function(void* p)
// {
//     int& param = *(int*)p;
//     param      = 100;
//     usleep(100000);
// }

static void test_function_exception1()
{
    throw runtime_error("this is to be expected");
}

static void test_function_exception2()
{
    throw out_of_range("this is to be expected");
}

TEST(util, async_exception)
{
    {
        future<void> func = async(&test_function_exception1);
        EXPECT_THROW(func.get(), std::runtime_error);
    }
    {
        future<void> func = async(&test_function_exception2);
        EXPECT_THROW(func.get(), std::out_of_range);
    }
}