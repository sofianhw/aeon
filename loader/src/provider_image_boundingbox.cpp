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

#include "provider_image_boundingbox.hpp"

using namespace nervana;
using namespace std;

image_boundingbox::image_boundingbox(nlohmann::json js)
    : provider_interface(js, 2)
    , image_config(js["image"])
    , bbox_config(js["boundingbox"])
    , image_extractor(image_config)
    , image_transformer(image_config)
    , image_loader(image_config)
    , image_factory(image_config)
    , bbox_extractor(bbox_config.label_map)
    , bbox_transformer(bbox_config)
    , bbox_loader(bbox_config)
{
    m_output_shapes.insert({"image", image_config.get_shape_type()});
    m_output_shapes.insert({"boundingbox", bbox_config.get_shape_type()});
}

void image_boundingbox::provide(int idx, encoded_record_list& in_buf, fixed_buffer_map& out_buf)
{
    vector<char>& datum_in  = in_buf.record(idx).element(0);
    vector<char>& target_in = in_buf.record(idx).element(1);

    char* datum_out  = out_buf["image"]->get_item(idx);
    char* target_out = out_buf["boundingbox"]->get_item(idx);

    if (datum_in.size() == 0)
    {
        std::stringstream ss;
        ss << "received encoded image with size 0, at idx " << idx;
        throw std::runtime_error(ss.str());
    }

    auto image_dec    = image_extractor.extract(datum_in.data(), datum_in.size());
    auto image_params = image_factory.make_params(image_dec);
    image_loader.load({datum_out}, image_transformer.transform(image_params, image_dec));

    // Process target data
    auto target_dec = bbox_extractor.extract(target_in.data(), target_in.size());
    bbox_loader.load({target_out}, bbox_transformer.transform(image_params, target_dec));
}
