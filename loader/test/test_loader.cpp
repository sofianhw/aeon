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

#include "gtest/gtest.h"
#include "loader.hpp"

#define private public

using namespace std;
using namespace nervana;

TEST(loader,test)
{
    nlohmann::json js = {{"type","image,label"},
                         {"image", {
                            {"height",128},
                            {"width",128},
                            {"channel_major",false},
                            {"flip_enable",true}}},
                         {"label", {
                              {"binary",true}
                          }
                         }};
    string config_string = js.dump();


    nervana::loader train_set = loader().config(config_string).batch_size(128).batch_count(BatchCount::INFINITE).create();
    auto partial_set = loader().config(config_string).batch_size(128).batch_count(10).create();
    auto valid_set = loader().config(config_string).batch_size(128).batch_count(BatchCount::ONCE).create();


    auto buf_names = train_set.get_keys_or_names();
    // buf_names == ['image', 'label']

    auto buf_shapes = train_set.get_shapes();
    // buf_shapes == {'image': (3, 32, 32), 'label': (1)}



    int count=0;
    for(const buffer_out& data : train_set)  // if d1 created with infinite, this will just keep going
    {
//        model_fit_one_iter(data);
//        data['image'], data['label'];  // for image, label provider
//        if(error < thresh) {
//            break;
//        }
        if(count++ > 10) break;
    }


//    all_errors = [];

//    for(auto data : valid_set)  // since d2 created with "once"
//    {
//        //all_errors.append(calc_batch_error(data))
//    }

    // now we've accumulated for the entire set:  (maybe a bit too much) Suppose 100 data, and batch_size 75

//    len(all_errors.size()) == 150
//    epoch_errors = all_errors[:len(d2)]

//    valid_set.reset();
}