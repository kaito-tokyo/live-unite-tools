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

#include "RenderingContext.hpp"

#include <cmath>

#include <opencv2/imgproc.hpp>

#include "../WebSocketServer/WebSocketServer.hpp"
#include "../TesseractReader/MatchTimerReader.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace KaitoTokyo {
namespace LiveUniteTools {

constexpr std::uint32_t EFFICIENTNET_INPUT_WIDTH = 224;
constexpr std::uint32_t EFFICIENTNET_INPUT_HEIGHT = 224;

RenderingContext::RenderingContext(obs_source_t *_source, const ILogger &_logger, unique_gs_effect_t gsMainEffect,
				   std::shared_ptr<WebSocketServer> _webSocketServer,
				   ThrottledTaskQueue &_mainTaskQueue, PluginConfig _pluginConfig,
				   long long _captureFps, std::uint32_t _width, std::uint32_t _height)
	: source(_source),
	  logger(_logger),
	  mainEffect(std::move(gsMainEffect)),
	  webSocketServer(std::move(_webSocketServer)),
	  mainTaskQueue(_mainTaskQueue),
	  captureFps(_captureFps),
	  width(_width),
	  height(_height),
	  pluginConfig(_pluginConfig),
	  bgrxSourceImage(make_unique_gs_texture(width, height, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  efficientNetRoiPosition{[this]() -> RoiPosition {
		  double widthScale = static_cast<double>(EFFICIENTNET_INPUT_WIDTH) / static_cast<double>(width);
		  double heightScale = static_cast<double>(EFFICIENTNET_INPUT_HEIGHT) / static_cast<double>(height);
		  double scale = std::min(widthScale, heightScale);

		  std::uint32_t scaledWidth = static_cast<std::uint32_t>(std::round(width * scale));
		  std::uint32_t scaledHeight = static_cast<std::uint32_t>(std::round(height * scale));

		  RoiPosition pos;
		  pos.left = (EFFICIENTNET_INPUT_WIDTH - scaledWidth) / 2;
		  pos.right = EFFICIENTNET_INPUT_WIDTH - pos.left;
		  pos.top = (EFFICIENTNET_INPUT_HEIGHT - scaledHeight) / 2;
		  pos.bottom = EFFICIENTNET_INPUT_HEIGHT - pos.top;
		  return pos;
	  }()},
	  bgrxSceneDetectorInput(make_unique_gs_texture(224, 224, GS_BGRX, 1, nullptr, GS_RENDER_TARGET)),
	  bgrxSceneDetectorInputReader(224, 224, GS_BGRX),
	  matchTimerRegion{static_cast<std::uint32_t>(pluginConfig.matchTimerRegion.x * width),
			   static_cast<std::uint32_t>(pluginConfig.matchTimerRegion.y * height),
			   static_cast<std::uint32_t>(pluginConfig.matchTimerRegion.width * width) & ~1u,
			   static_cast<std::uint32_t>(pluginConfig.matchTimerRegion.height * height) & ~1u},
	  hsvxMatchTimerHistory{BridgeUtils::make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height,
								    GS_BGRX, 1, nullptr, GS_RENDER_TARGET),
				BridgeUtils::make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height,
								    GS_BGRX, 1, nullptr, GS_RENDER_TARGET),
				BridgeUtils::make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height,
								    GS_BGRX, 1, nullptr, GS_RENDER_TARGET),
				BridgeUtils::make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height,
								    GS_BGRX, 1, nullptr, GS_RENDER_TARGET),
				BridgeUtils::make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height,
								    GS_BGRX, 1, nullptr, GS_RENDER_TARGET)},
	  hsvxMatchTimerReader(matchTimerRegion.width, matchTimerRegion.height, GS_BGRX),
	  r8MedianMatchTimer(make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height, GS_R8, 1, nullptr,
						    GS_RENDER_TARGET)),
	  r8MedianMatchTimerReader(matchTimerRegion.width, matchTimerRegion.height, GS_R8),
	  contextClassifier(contextClassifierNet)
{
	contextClassifierNet.opt.num_threads = 2;
	contextClassifierNet.opt.use_local_pool_allocator = true;
	contextClassifierNet.opt.openmp_blocktime = 1;

	unique_bfree_char_t paramPath = unique_obs_module_file("models/ContextClassifier.ncnn.param");
	if (!paramPath) {
		throw std::runtime_error("Failed to find model param file");
	}
	unique_bfree_char_t binPath = unique_obs_module_file("models/ContextClassifier.ncnn.bin");
	if (!binPath) {
		throw std::runtime_error("Failed to find model bin file");
	}

	if (contextClassifierNet.load_param(paramPath.get()) != 0) {
		throw std::runtime_error("Failed to load model param");
	}
	if (contextClassifierNet.load_model(binPath.get()) != 0) {
		throw std::runtime_error("Failed to load model bin");
	}
}

RenderingContext::~RenderingContext() noexcept {}

void RenderingContext::videoTick(float)
{
	doesNextVideoRenderReceiveNewFrame = true;
}

void RenderingContext::videoRender()
{
	if (doesNextVideoRenderReceiveNewFrame) {
		doesNextVideoRenderReceiveNewFrame = false;
		videoRenderNewFrame();
	}

	//obs_source_skip_video_filter(source);
	while (gs_effect_loop(mainEffect.gsEffect.get(), "Draw")) {
		gs_effect_set_texture(mainEffect.textureImage, r8MedianMatchTimer.get());
		gs_draw_sprite(r8MedianMatchTimer.get(), 0, width, height);
	}
}

void RenderingContext::videoRenderNewFrame()
{
	hsvxMatchTimerReader.sync();
	r8MedianMatchTimerReader.sync();

	mainTaskQueue.push([self = shared_from_this()](const ThrottledTaskQueue::CancellationToken &token) {
		if (token->load()) {
			return;
		}

		auto &r8MedianMatchTimerReader = self->r8MedianMatchTimerReader;

		cv::Mat medianMatchTimerImage(r8MedianMatchTimerReader.getHeight(), r8MedianMatchTimerReader.getWidth(),
					      CV_8UC1,
					      static_cast<void *>(r8MedianMatchTimerReader.getBuffer().data()));

		cv::Mat invertedImage;
		cv::bitwise_not(medianMatchTimerImage, invertedImage);

		cv::Mat binaryImage;
		cv::threshold(invertedImage, binaryImage, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        unique_bfree_char_t tessdataPath = unique_obs_module_file("tessdata");
		MatchTimerReader matchTimerReader(tessdataPath.get());
		std::string matchTime = matchTimerReader.read(binaryImage);

		self->logger.info("Match Time: {}", matchTime);
	});

	mainEffect.drawSource(bgrxSourceImage, source);

	hsvxMatchTimerHistoryIndex = (hsvxMatchTimerHistoryIndex + 1) % hsvxMatchTimerHistory.size();
	auto &hsvxCurrentMatchTimer = hsvxMatchTimerHistory[hsvxMatchTimerHistoryIndex];

	mainEffect.convertToHSV(hsvxCurrentMatchTimer, bgrxSourceImage, static_cast<float>(matchTimerRegion.x),
				static_cast<float>(matchTimerRegion.y));

	mainEffect.medianFiltering5(r8MedianMatchTimer, hsvxMatchTimerHistory);

	hsvxMatchTimerReader.stage(hsvxCurrentMatchTimer.get());
	r8MedianMatchTimerReader.stage(r8MedianMatchTimer.get());
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	return frame;
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
