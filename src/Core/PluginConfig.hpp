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

namespace KaitoTokyo {
namespace LiveUniteTools {

struct PluginConfigRegion {
	double x;
	double y;
	double width;
	double height;
};

struct PluginConfig {
	PluginConfigRegion matchTimerRegion = {800.0 / 1920.0, 20.0 / 1080.0, 320.0 / 1920.0, 100.0 / 1080.0};
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo
