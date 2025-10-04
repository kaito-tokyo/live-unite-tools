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

#include <algorithm>
#include <cctype>
#include <memory>
#include <regex>

namespace KaitoTokyo {
namespace LiveUniteTools {

MatchTimerReader::MatchTimerReader(const char *tessdataPath)
{
	if (api.Init(tessdataPath, "eng")) {
		throw std::runtime_error("Could not initialize tesseract.");
	}

	api.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	api.SetVariable("tessedit_char_whitelist", "0123456789:");
}

MatchTimerReader::~MatchTimerReader() noexcept
try {
	api.End();
} catch (...) {
}

std::string MatchTimerReader::read(cv::Mat &lumaData)
{
	api.SetImage(lumaData.data, static_cast<int>(lumaData.cols), static_cast<int>(lumaData.rows), 1,
		     static_cast<int>(lumaData.step));

	std::unique_ptr<char[]> outText(api.GetUTF8Text());

	if (!outText) {
		return "";
	}

	std::string rawText = outText.get();

	std::string cleanedText = rawText;
	cleanedText.erase(std::remove_if(cleanedText.begin(), cleanedText.end(),
					 [](unsigned char c) { return std::isspace(c); }),
			  cleanedText.end());

	const std::regex timer_pattern(R"(^0\d:\d{2}$)");

	if (std::regex_match(cleanedText, timer_pattern)) {
		return cleanedText;
	} else {
		return "";
	}
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
