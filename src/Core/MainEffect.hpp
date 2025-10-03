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

#include <obs.h>

#include "BridgeUtils/GsUnique.hpp"

namespace KaitoTokyo {
namespace LiveUniteTools {

namespace MainEffectDetail {

inline gs_eparam_t *getEffectParam(const BridgeUtils::unique_gs_effect_t &gsEffect, const char *name)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(gsEffect.get(), name);

	if (!param) {
		throw std::runtime_error("Failed to get effect param: " + std::string(name));
	}

	return param;
}

struct TransformStateGuard {
	TransformStateGuard()
	{
		gs_viewport_push();
		gs_projection_push();
		gs_matrix_push();
		gs_blend_state_push();
	}
	~TransformStateGuard()
	{
		gs_blend_state_push();
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

} // namespace MainEffectDetail

class MainEffect {
public:
	const BridgeUtils::unique_gs_effect_t gsEffect;

	gs_eparam_t *const textureImage;

	MainEffect(BridgeUtils::unique_gs_effect_t _gsEffect)
		: gsEffect(std::move(_gsEffect)),
		  textureImage(MainEffectDetail::getEffectParam(gsEffect, "image"))
	{
	}

	MainEffect(const MainEffect &) = delete;
	MainEffect(MainEffect &&) = delete;
	MainEffect &operator=(const MainEffect &) = delete;
	MainEffect &operator=(MainEffect &&) = delete;

	void drawSource(BridgeUtils::unique_gs_texture_t &targetTexture, obs_source_t *source)
	{
		MainEffectDetail::RenderTargetGuard renderTargetGuard;
		MainEffectDetail::TransformStateGuard transformGuard;

		std::uint32_t targetWidth = gs_texture_get_width(targetTexture.get());
		std::uint32_t targetHeight = gs_texture_get_height(targetTexture.get());

		gs_set_viewport(0, 0, static_cast<int>(targetWidth), static_cast<int>(targetHeight));
		gs_ortho(0.0f, static_cast<float>(targetWidth), 0.0f, static_cast<float>(targetHeight), -100.0f,
			 100.0f);
		gs_matrix_identity();

		gs_set_render_target_with_color_space(targetTexture.get(), nullptr, GS_CS_709_EXTENDED);
		while (gs_effect_loop(obs_get_base_effect(OBS_EFFECT_DEFAULT), "Draw")) {
			obs_source_video_render(source);
		}
	}

	// void convertToGrayscale(unique_gs_texture_t &targetTexture, unique_gs_texture_t &sourceTexture, float x = 0.0f,
	// 	  float y = 0.0f, std::uint32_t width = 0, std::uint32_t height = 0)
	// {
	// 	MainEffectDetail::RenderTargetGuard renderTargetGuard;
	// 	MainEffectDetail::TransformStateGuard transformGuard;

	// 	std::uint32_t targetWidth = gs_texture_get_width(targetTexture.get());
	// 	std::uint32_t targetHeight = gs_texture_get_height(targetTexture.get());

	// 	gs_set_viewport(0, 0, static_cast<int>(targetWidth), static_cast<int>(targetHeight));
	// 	gs_ortho(0.0f, static_cast<float>(targetWidth), 0.0f, static_cast<float>(targetHeight), -100.0f,
	// 		 100.0f);
	// 	gs_matrix_identity();
	// 	gs_matrix_translate3f(x, y, 0.0f);

	// 	while (gs_effect_loop(gsEffect.get(), "ConvertToGrayscale")) {
	// 		gs_effect_set_texture(textureImage, sourceTexture.get());
	// 		gs_draw_sprite(nullptr, 0, width, height);
	// 	}
	// }
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo
