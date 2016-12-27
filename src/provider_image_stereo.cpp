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

#include "provider_image_stereo.hpp"

using namespace nervana;
using namespace std;

image_stereo_blob::image_stereo_blob(nlohmann::json js)
    : provider_interface(js, 3)
    , image_config(js["stereo_image"])
    , target_config(js["blob"])
    , image_extractor(image_config)
    , image_transformer(image_config)
    , image_loader(image_config)
    , image_factory(image_config)
    , target_extractor(target_config)
    ,
    //    target_transformer(target_config),
    target_loader(target_config)
{
    m_output_shapes.insert({"image_left", image_config.get_shape_type()});
    m_output_shapes.insert({"image_right", image_config.get_shape_type()});
    m_output_shapes.insert({"depthmap", target_config.get_shape_type()});
}

void image_stereo_blob::provide(int idx, encoded_record_list& in_buf, fixed_buffer_map& out_buf)
{
    std::vector<char>& l_in      = in_buf.record(idx).element(0);
    std::vector<char>& r_in      = in_buf.record(idx).element(1);
    std::vector<char>& target_in = in_buf.record(idx).element(2);

    char* l_out      = out_buf["image_left"]->get_item(idx);
    char* r_out      = out_buf["image_right"]->get_item(idx);
    char* target_out = out_buf["depthmap"]->get_item(idx);

    auto l_dec         = image_extractor.extract(l_in.data(), l_in.size());
    auto r_dec         = image_extractor.extract(r_in.data(), r_in.size());
    auto image_params  = image_factory.make_params(l_dec);
    auto l_transformed = image_transformer.transform(image_params, l_dec);
    auto r_transformed = image_transformer.transform(image_params, r_dec);
    image_loader.load({l_out}, l_transformed);
    image_loader.load({r_out}, r_transformed);

    // Process target data
    auto target_dec = target_extractor.extract(target_in.data(), target_in.size());
    //    auto target_transformed = target_transformer.transform(image_params, target_dec);
    target_loader.load({target_out}, target_dec);
}
