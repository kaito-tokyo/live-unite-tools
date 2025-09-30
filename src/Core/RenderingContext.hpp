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

#include <atomic>
#include <cstdint>

#include "BridgeUtils/AsyncTextureReader.hpp"
#include "BridgeUtils/GsUnique.hpp"
#include "BridgeUtils/ILogger.hpp"

namespace KaitoTokyo {
namespace LiveUniteTools {

struct RoiPosition {
	std::uint32_t left;
	std::uint32_t right;
	std::uint32_t top;
	std::uint32_t bottom;
};

class RenderingContext : public std::enable_shared_from_this<RenderingContext> {
private:
	obs_source_t *const source;
	const BridgeUtils::ILogger &logger;

public:
	const std::uint32_t width;
	const std::uint32_t height;

	const BridgeUtils::unique_gs_texture_t bgrxSourceImage;

	const RoiPosition efficientNetRoiPosition;

	const BridgeUtils::unique_gs_texture_t bgrxSceneDetectorInput;
	BridgeUtils::AsyncTextureReader bgrxSceneDetectorInputReader;

	std::uint64_t lastFrameTimestamp = 0;
	std::atomic<bool> doesNextVideoRenderReceiveNewFrame = false;

public:
	RenderingContext(obs_source_t *source, const BridgeUtils::ILogger &logger, std::uint32_t width,
			 std::uint32_t height);
	~RenderingContext() noexcept;

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);

private:
	void videoRenderNewFrame();
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo
