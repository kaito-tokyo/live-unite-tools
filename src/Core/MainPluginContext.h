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

#include <stdint.h>

#include <obs.h>

#ifdef __cplusplus

#include <future>

#include "BridgeUtils/ILogger.hpp"

#include "RenderingContext.hpp"

namespace KaitoTokyo {
namespace LiveUniteTools {

class MainPluginContext : public std::enable_shared_from_this<MainPluginContext> {
private:
	obs_source_t *const source;
	const BridgeUtils::ILogger &logger;
	std::shared_future<std::string> latestVersionFuture;

	ncnn::Net contextClassifierNet;

	std::shared_ptr<RenderingContext> renderingContext;

public:
	MainPluginContext(obs_data_t *settings, obs_source_t *source,
			  std::shared_future<std::string> latestVersionFuture, const BridgeUtils::ILogger &logger);
	void startup() noexcept;
	void shutdown() noexcept;
	~MainPluginContext() noexcept;

	uint32_t getWidth() const noexcept;
	uint32_t getHeight() const noexcept;

	static void getDefaults(obs_data_t *data);

	obs_properties_t *getProperties();
	void update(obs_data_t *settings);
	void activate();
	void deactivate();
	void show();
	void hide();

	void videoTick(float seconds);
	void videoRender();
	obs_source_frame *filterVideo(obs_source_frame *frame);
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo

extern "C" {
#endif // __cplusplus

bool main_plugin_context_module_load(void);
void main_plugin_context_module_unload(void);

const char *main_plugin_context_get_name(void *type_data);
void *main_plugin_context_create(obs_data_t *settings, obs_source_t *source);
void main_plugin_context_destroy(void *data);
uint32_t main_plugin_context_get_width(void *data);
uint32_t main_plugin_context_get_height(void *data);
void main_plugin_context_get_defaults(obs_data_t *data);
obs_properties_t *main_plugin_context_get_properties(void *data);
void main_plugin_context_update(void *data, obs_data_t *settings);
void main_plugin_context_activate(void *data);
void main_plugin_context_deactivate(void *data);
void main_plugin_context_show(void *data);
void main_plugin_context_hide(void *data);
void main_plugin_context_video_tick(void *data, float seconds);
void main_plugin_context_video_render(void *data, gs_effect_t *effect);
struct obs_source_frame *main_plugin_context_filter_video(void *data, struct obs_source_frame *frame);

#ifdef __cplusplus
}
#endif // __cplusplus
