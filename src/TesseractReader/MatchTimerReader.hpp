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

#pragma once

#include <cstdint>
#include <string>

#include <tesseract/baseapi.h>
#include <opencv2/core.hpp>

namespace KaitoTokyo {
namespace LiveUniteTools {

class MatchTimerReader {
private:
	tesseract::TessBaseAPI api;

public:
	MatchTimerReader(const char *tessdataPath);
	~MatchTimerReader() noexcept;

	std::string read(cv::Mat &lumaData);
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo
