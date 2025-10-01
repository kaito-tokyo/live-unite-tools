/*
Live Background Removal Lite
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

#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

#include <net.h>

#include "EfficientNet.hpp"

namespace KaitoTokyo {
namespace LiveUniteTools {

const std::vector<std::string> contextClassifierClassNames = {
	"GoalDefense",  "LanePhase",      "LegendaryFightPhase",     "OutOfGame",      "PreparationPhase",
	"ResultScreen", "ScoringAttempt", "SecondaryObjectiveFight", "WaitForRespawn",
};

class ContextClassifier {
private:
	EfficientNet efficientNet;

public:
	ContextClassifier(const ncnn::Net &net)
		: efficientNet(net, static_cast<int>(contextClassifierClassNames.size()))
	{
	}

	void process(const std::uint8_t *bgraData) { efficientNet.process(bgraData); }

	std::string getInferredClassName()
	{
		auto &logits = efficientNet.getOutputBuffer();
		auto maxIt = std::max_element(logits.begin(), logits.end());
		auto index = std::distance(logits.begin(), maxIt);
		return contextClassifierClassNames[index];
	}
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo
