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
				   std::shared_ptr<WebSocketServer> _webSocketServer, PluginConfig _pluginConfig,
				   std::uint32_t _width, std::uint32_t _height)
	: source(_source),
	  logger(_logger),
	  mainEffect(std::move(gsMainEffect)),
	  webSocketServer(std::move(_webSocketServer)),
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
	  hsvxMatchTimer(make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height, GS_BGRX, 1, nullptr,
						GS_RENDER_TARGET)),
	  hsvxMatchTimerReader(matchTimerRegion.width, matchTimerRegion.height, GS_BGRX),
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

	// obs_source_skip_video_filter(source);

	while (gs_effect_loop(mainEffect.gsEffect.get(), "Draw")) {
		gs_effect_set_texture(mainEffect.textureImage, hsvxMatchTimer.get());
		gs_draw_sprite(hsvxMatchTimer.get(), 0, 0, 0);
	}

	const int sizes[]{static_cast<int>(hsvxMatchTimerReader.getHeight()),
			  static_cast<int>(hsvxMatchTimerReader.getWidth())};
	cv::Mat hsvx_image(2, sizes, CV_8UC4, static_cast<void *>(hsvxMatchTimerReader.getBuffer().data()));

	cv::Mat v_channel_image;
	cv::extractChannel(hsvx_image, v_channel_image, 2);

	cv::Mat blurred_image;
	cv::GaussianBlur(v_channel_image, blurred_image, cv::Size(3, 3), 0);

	cv::Mat inverted_image;
	cv::bitwise_not(blurred_image, inverted_image);

	cv::Mat binary_image;
	cv::adaptiveThreshold(inverted_image, binary_image, 255,
			      cv::ADAPTIVE_THRESH_GAUSSIAN_C, // ガウシアンで重み付け
			      cv::THRESH_BINARY,              // 白を背景、黒を文字に
			      11,                             // ブロックサイズ (奇数である必要あり。調整推奨)
			      2);                             // 定数C (閾値から引かれる値。調整推奨)

	const std::uint8_t *planarData[] = {inverted_image.data};

	unique_gs_texture_t r8MatchTimerDebug =
		make_unique_gs_texture(matchTimerRegion.width, matchTimerRegion.height, GS_R8, 1, planarData, 0);

	while (gs_effect_loop(mainEffect.gsEffect.get(), "Draw")) {
		gs_effect_set_texture(mainEffect.textureImage, r8MatchTimerDebug.get());
		gs_draw_sprite(r8MatchTimerDebug.get(), 0, 0, 0);
	}

	MatchTimerReader matchTimerReader;
	std::string matchTime = matchTimerReader.read(binary_image);

	logger.info("Match Time: {}", matchTime);
}

void RenderingContext::videoRenderNewFrame()
{
	hsvxMatchTimerReader.sync();

	mainEffect.drawSource(bgrxSourceImage, source);
	mainEffect.convertToHSV(hsvxMatchTimer, bgrxSourceImage, static_cast<float>(matchTimerRegion.x),
				static_cast<float>(matchTimerRegion.y));

	hsvxMatchTimerReader.stage(hsvxMatchTimer.get());
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	return frame;
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
