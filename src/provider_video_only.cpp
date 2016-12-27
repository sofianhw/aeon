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

#include "provider_video_only.hpp"

using namespace nervana;
using namespace std;

video_only::video_only(nlohmann::json js)
    : provider_interface(js, 1)
    , video_config(js["video"])
    , video_extractor(video_config)
    , video_transformer(video_config)
    , video_loader(video_config)
    , frame_factory(video_config.frame)
{
    m_output_shapes.insert({"video", video_config.get_shape_type()});
}

void video_only::provide(int idx, encoded_record_list& in_buf, fixed_buffer_map& out_buf)
{
    vector<char>& datum_in  = in_buf.record(idx).element(0);
    char*         datum_out = out_buf["video"]->get_item(idx);

    if (datum_in.size() == 0)
    {
        std::stringstream ss;
        ss << "received encoded video with size 0, at idx " << idx;
        throw std::runtime_error(ss.str());
    }

    // Process video data
    auto video_dec    = video_extractor.extract(datum_in.data(), datum_in.size());
    auto frame_params = frame_factory.make_params(video_dec);
    video_loader.load({datum_out}, video_transformer.transform(frame_params, video_dec));
}
