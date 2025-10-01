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

#include "../WebSocketServer/WebSocketServer.hpp"

using namespace KaitoTokyo::BridgeUtils;

namespace {

struct TransformStateGuard {
	TransformStateGuard()
	{
		gs_viewport_push();
		gs_projection_push();
		gs_matrix_push();
	}
	~TransformStateGuard()
	{
		gs_matrix_pop();
		gs_projection_pop();
		gs_viewport_pop();
	}
};

struct RenderTargetGuard {
	gs_texture_t *previousRenderTarget;
	gs_zstencil_t *previousZStencil;
	gs_color_space previousColorSpace;

	RenderTargetGuard()
		: previousRenderTarget(gs_get_render_target()),
		  previousZStencil(gs_get_zstencil_target()),
		  previousColorSpace(gs_get_color_space())
	{
	}

	~RenderTargetGuard()
	{
		gs_set_render_target_with_color_space(previousRenderTarget, previousZStencil, previousColorSpace);
	}
};

} // namespace

namespace KaitoTokyo {
namespace LiveUniteTools {

constexpr std::uint32_t EFFICIENTNET_INPUT_WIDTH = 224;
constexpr std::uint32_t EFFICIENTNET_INPUT_HEIGHT = 224;

RenderingContext::RenderingContext(obs_source_t *_source, const ILogger &_logger, std::uint32_t _width,
				   std::uint32_t _height, std::shared_ptr<WebSocketServer> _webSocketServer)
	: source(_source),
	  logger(_logger),
	  width(_width),
	  height(_height),
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
	  contextClassifier(contextClassifierNet),
	  webSocketServer(_webSocketServer)
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

void RenderingContext::videoTick(float) {}

void RenderingContext::videoRender()
{
	if (doesNextVideoRenderReceiveNewFrame) {
		doesNextVideoRenderReceiveNewFrame = false;
		videoRenderNewFrame();
	}

	obs_source_skip_video_filter(source);
}

void RenderingContext::videoRenderNewFrame()
{
	bgrxSceneDetectorInputReader.sync();
	contextClassifier.process(bgrxSceneDetectorInputReader.getBuffer().data());
	// Send inferred class name via WebSocket
	if (webSocketServer) {
		webSocketServer->broadcast(contextClassifier.getInferredClassName());
	}

	RenderTargetGuard renderTargetGuard;
	TransformStateGuard transformGuard;

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	if (!obs_source_process_filter_begin(source, GS_BGRX, OBS_ALLOW_DIRECT_RENDERING)) {
		logger.error("Could not begin processing filter");
		obs_source_skip_video_filter(source);
		return;
	}

	const auto &pos = efficientNetRoiPosition;
	gs_set_viewport(0, 0, static_cast<int>(EFFICIENTNET_INPUT_WIDTH), static_cast<int>(EFFICIENTNET_INPUT_HEIGHT));
	gs_ortho(0.0f, static_cast<float>(EFFICIENTNET_INPUT_WIDTH), 0.0f,
		 static_cast<float>(EFFICIENTNET_INPUT_HEIGHT), -100.0f, 100.0f);
	gs_matrix_identity();
	gs_matrix_translate3f(static_cast<float>(pos.left), static_cast<float>(pos.top), 0.0f);

	gs_set_render_target_with_color_space(bgrxSceneDetectorInput.get(), nullptr, GS_CS_709_EXTENDED);

	vec4 grayColor{0.5f, 0.5f, 0.5f, 1.0f};
	gs_clear(GS_CLEAR_COLOR, &grayColor, 1.0f, 0);

	obs_source_process_filter_end(source, effect, pos.right - pos.left, pos.bottom - pos.top);

	bgrxSceneDetectorInputReader.stage(bgrxSceneDetectorInput.get());

	logger.info("Best: {}", contextClassifier.getInferredClassName());
}

obs_source_frame *RenderingContext::filterVideo(obs_source_frame *frame)
{
	if (!frame) {
		return nullptr;
	}

	if (frame->timestamp > lastFrameTimestamp) {
		doesNextVideoRenderReceiveNewFrame = true;
		lastFrameTimestamp = frame->timestamp;
	}

	return frame;
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
