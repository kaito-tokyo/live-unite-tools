/*
Live Unite Tools
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "MatchTimerReader.hpp"

#include "../BridgeUtils/ObsUnique.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo {
namespace LiveUniteTools {

MatchTimerReader::MatchTimerReader() {
    unique_bfree_char_t tessdataPath = unique_obs_module_file("tessdata");
    if (api.Init(tessdataPath.get(), "eng")) {
        throw std::runtime_error("Could not initialize tesseract.");
    }

    api.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    api.SetVariable("tessedit_char_whitelist", "0123456789:");
}

MatchTimerReader::~MatchTimerReader() noexcept try {
    api.End();
} catch (...) {
}

std::string MatchTimerReader::read(cv::Mat &lumaData) {
    api.SetImage(lumaData.data, lumaData.cols, lumaData.rows, 1, lumaData.step);
    return api.GetUTF8Text();
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
