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

#include <future>
#include <stdexcept>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include "BridgeUtils/GsUnique.hpp"
#include "BridgeUtils/ILogger.hpp"
#include "BridgeUtils/ObsUnique.hpp"

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::BridgeUtils;

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

std::uint32_t MainPluginContext::getWidth() const noexcept {
	return 0;
}

std::uint32_t MainPluginContext::getHeight() const noexcept {
	return 0;
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

void MainPluginContext::videoTick(float) {}

void MainPluginContext::videoRender() {}

obs_source_frame *MainPluginContext::filterVideo(struct obs_source_frame *frame)
try {
	return frame;
} catch (const std::exception &e) {
	logger.error("Failed to create rendering context: {}", e.what());
	return frame;
} catch (...) {
	logger.error("Failed to create rendering context: unknown error");
	return frame;
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
