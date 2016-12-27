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

#include "provider_interface.hpp"
#include "etl_video.hpp"

namespace nervana
{
    class video_only;
}

class nervana::video_only : public provider_interface
{
public:
    video_only(nlohmann::json js);
    void provide(int idx, nervana::encoded_record_list& in_buf, nervana::fixed_buffer_map& out_buf);

private:
    video::config        video_config;
    video::extractor     video_extractor;
    video::transformer   video_transformer;
    video::loader        video_loader;
    image::param_factory frame_factory;
};
