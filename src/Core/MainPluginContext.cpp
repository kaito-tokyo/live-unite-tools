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

#include "MainPluginContext.h"

#include <fstream>
#include <future>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "BridgeUtils/GsUnique.hpp"
#include "BridgeUtils/ILogger.hpp"
#include "BridgeUtils/ObsUnique.hpp"

#include "Core/MainEffect.hpp"

using namespace KaitoTokyo::BridgeUtils;
using json = nlohmann::json;

namespace KaitoTokyo {
namespace LiveUniteTools {

MainPluginContext::MainPluginContext(obs_data_t *settings, obs_source_t *_source,
				     std::shared_future<std::string> _latestVersionFuture,
				     const BridgeUtils::ILogger &_logger)
	: source{_source},
	  logger(_logger),
	  latestVersionFuture{_latestVersionFuture}
{
	update(settings);
}

void MainPluginContext::startup() noexcept {}

void MainPluginContext::shutdown() noexcept {}

MainPluginContext::~MainPluginContext() noexcept {}

std::uint32_t MainPluginContext::getWidth() const noexcept
{
	return renderingContext ? renderingContext->width : 0;
}

std::uint32_t MainPluginContext::getHeight() const noexcept
{
	return renderingContext ? renderingContext->height : 0;
}

void MainPluginContext::getDefaults(obs_data_t *) {}

obs_properties_t *MainPluginContext::getProperties()
{
	return obs_properties_create();
}

void MainPluginContext::update(obs_data_t *) {}

void MainPluginContext::activate() {}

void MainPluginContext::deactivate() {}

void MainPluginContext::show() {}

void MainPluginContext::hide() {}

void MainPluginContext::videoTick(float seconds)
{
	obs_source_t *target = obs_filter_get_target(source);
	uint32_t targetWidth = obs_source_get_width(target);
	uint32_t targetHeight = obs_source_get_height(target);

	if (targetWidth == 0 || targetHeight == 0) {
		targetWidth = obs_source_get_base_width(target);
		targetHeight = obs_source_get_base_height(target);
	}

	if (targetWidth == 0 || targetHeight == 0) {
		logger.debug("Target source has zero width or height, skipping video tick");
		return;
	}

	if (!renderingContext || renderingContext->width != targetWidth || renderingContext->height != targetHeight) {
		GraphicsContextGuard guard;
		renderingContext = makeRenderingContext(targetWidth, targetHeight);
		GsUnique::drain();
	}

	if (renderingContext) {
		renderingContext->videoTick(seconds);
	}
}

void MainPluginContext::videoRender()
{
	if (!renderingContext) {
		logger.debug("Rendering context is not initialized, skipping video render");
		obs_source_skip_video_filter(source);
		return;
	}

	renderingContext->videoRender();
}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
try {
	if (renderingContext) {
		return renderingContext->filterVideo(frame);
	} else {
		return frame;
	}
} catch (const std::exception &e) {
	logger.error("Failed to create rendering context: {}", e.what());
	return frame;
} catch (...) {
	logger.error("Failed to create rendering context: unknown error");
	return frame;
}

std::shared_ptr<RenderingContext> MainPluginContext::makeRenderingContext(std::uint32_t targetWidth,
									  std::uint32_t targetHeight)
{
	unique_bfree_char_t mainEffectPath(unique_obs_module_file("effect/main.effect"));
	unique_gs_effect_t gsMainEffect = make_unique_gs_effect_from_file(mainEffectPath);
	MainEffect mainEffect(std::move(gsMainEffect));

	std::shared_ptr<WebSocketServer> webSocketServer = WebSocketServer::getSharedWebSocketServer();

	PluginConfig defaultConfig;

	return std::make_shared<RenderingContext>(source, logger, mainEffect, std::move(webSocketServer),
						  std::move(defaultConfig), targetWidth, targetHeight);
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
