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

#include <stdexcept>

#include <curl/curl.h>

#include <obs-module.h>

#include "BridgeUtils/GsUnique.hpp"
#include "BridgeUtils/ObsLogger.hpp"

#include "../UpdateChecker/UpdateChecker.hpp"

using namespace KaitoTokyo::BridgeUtils;
using namespace KaitoTokyo::LiveUniteTools;

namespace {

std::shared_future<std::string> latestVersionFuture;

inline const ILogger &logger()
{
	static const ObsLogger instance("[" PLUGIN_NAME "] ");
	return instance;
}

} // namespace

bool main_plugin_context_module_load()
try {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	latestVersionFuture =
		std::async(std::launch::async, [] {
			return KaitoTokyo::UpdateChecker::fetchLatestVersion(
				"https://kaito-tokyo.github.io/live-unite-tools/metadata/latest-version.txt");
		}).share();
	logger().info("plugin loaded successfully (version " PLUGIN_VERSION ")");
	return true;
} catch (const std::exception &e) {
	logger().error("Failed to load main plugin context: %s", e.what());
	return false;
} catch (...) {
	logger().error("Failed to load main plugin context: unknown error");
	return false;
}

void main_plugin_context_module_unload()
try {
	GraphicsContextGuard guard;
	GsUnique::drain();
	logger().info("plugin unloaded");
} catch (const std::exception &e) {
	logger().error("Failed to unload main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to unload main plugin context: unknown error");
}

const char *main_plugin_context_get_name(void *type_data)
{
	UNUSED_PARAMETER(type_data);
	return obs_module_text("pluginName");
}

void *main_plugin_context_create(obs_data_t *settings, obs_source_t *source)
try {
	GraphicsContextGuard guard;
	auto self = std::make_shared<MainPluginContext>(settings, source, latestVersionFuture, logger());
	self->startup();
	return new std::shared_ptr<MainPluginContext>(self);
} catch (const std::exception &e) {
	logger().error("Failed to create main plugin context: %s", e.what());
	return nullptr;
} catch (...) {
	logger().error("Failed to create main plugin context: unknown error");
	return nullptr;
}

void main_plugin_context_destroy(void *data)
try {
	if (!data) {
		logger().error("main_plugin_context_destroy called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->shutdown();
	delete self;

	GraphicsContextGuard guard;
	GsUnique::drain();
} catch (const std::exception &e) {
	logger().error("Failed to destroy main plugin context: %s", e.what());

	GraphicsContextGuard guard;
	GsUnique::drain();
} catch (...) {
	logger().error("Failed to destroy main plugin context: unknown error");

	GraphicsContextGuard guard;
	GsUnique::drain();
}

std::uint32_t main_plugin_context_get_width(void *data)
{
	if (!data) {
		logger().error("main_plugin_context_get_width called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getWidth();
}

std::uint32_t main_plugin_context_get_height(void *data)
{
	if (!data) {
		logger().error("main_plugin_context_get_height called with null data");
		return 0;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getHeight();
}

void main_plugin_context_get_defaults(obs_data_t *data)
{
	MainPluginContext::getDefaults(data);
}

obs_properties_t *main_plugin_context_get_properties(void *data)
try {
	if (!data) {
		logger().error("main_plugin_context_get_properties called with null data");
		return obs_properties_create();
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	return self->get()->getProperties();
} catch (const std::exception &e) {
	logger().error("Failed to get properties: %s", e.what());
	return obs_properties_create();
} catch (...) {
	logger().error("Failed to get properties: unknown error");
	return obs_properties_create();
}

void main_plugin_context_update(void *data, obs_data_t *settings)
try {
	if (!data) {
		logger().error("main_plugin_context_update called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->update(settings);
} catch (const std::exception &e) {
	logger().error("Failed to update main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to update main plugin context: unknown error");
}

void main_plugin_context_activate(void *data)
try {
	if (!data) {
		logger().error("main_plugin_context_activate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->activate();
} catch (const std::exception &e) {
	logger().error("Failed to activate main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to activate main plugin context: unknown error");
}

void main_plugin_context_deactivate(void *data)
try {
	if (!data) {
		logger().error("main_plugin_context_deactivate called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->deactivate();
} catch (const std::exception &e) {
	logger().error("Failed to deactivate main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to deactivate main plugin context: unknown error");
}

void main_plugin_context_show(void *data)
try {
	if (!data) {
		logger().error("main_plugin_context_show called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->show();
} catch (const std::exception &e) {
	logger().error("Failed to show main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to show main plugin context: unknown error");
}

void main_plugin_context_hide(void *data)
try {
	if (!data) {
		logger().error("main_plugin_context_hide called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->hide();
} catch (const std::exception &e) {
	logger().error("Failed to hide main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to hide main plugin context: unknown error");
}

void main_plugin_context_video_tick(void *data, float seconds)
try {
	if (!data) {
		logger().error("main_plugin_context_video_tick called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->videoTick(seconds);
} catch (const std::exception &e) {
	logger().error("Failed to tick main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to tick main plugin context: unknown error");
}

void main_plugin_context_video_render(void *data, gs_effect_t *_unused)
try {
	UNUSED_PARAMETER(_unused);

	if (!data) {
		logger().error("main_plugin_context_video_render called with null data");
		return;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	self->get()->videoRender();
	GsUnique::drain();
} catch (const std::exception &e) {
	logger().error("Failed to render video in main plugin context: %s", e.what());
} catch (...) {
	logger().error("Failed to render video in main plugin context: unknown error");
}

struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame)
try {
	if (!data) {
		logger().error("main_plugin_context_filter_video called with null data");
		return frame;
	}

	auto self = static_cast<std::shared_ptr<MainPluginContext> *>(data);
	obs_source_frame *result = self->get()->filterVideo(frame);
	return result;
} catch (const std::exception &e) {
	logger().error("Failed to filter video in main plugin context: %s", e.what());
	return frame;
} catch (...) {
	logger().error("Failed to filter video in main plugin context: unknown error");
	return frame;
}
