/*
Live Background Removal Lite
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

#include "EfficientNet.hpp"

#include <cstddef>
#include <vector>

namespace KaitoTokyo {
namespace LiveUniteTools {

namespace {

inline void copyDataToMatNaive(ncnn::Mat &inputMat, const std::uint8_t *bgra_data)
{
	float *r_channel = inputMat.channel(0);
	float *g_channel = inputMat.channel(1);
	float *b_channel = inputMat.channel(2);

	for (int i = 0; i < EfficientNet::PIXEL_COUNT; i++) {
		r_channel[i] = (static_cast<float>(bgra_data[i * 4 + 2]) - 123.675f) / 58.395f;
		g_channel[i] = (static_cast<float>(bgra_data[i * 4 + 1]) - 116.28f) / 57.12f;
		b_channel[i] = (static_cast<float>(bgra_data[i * 4 + 0]) - 103.53f) / 57.375f;
	}
}

#ifdef SELFIE_SEGMENTER_HAVE_NEON

inline void copyDataToMatNeon(ncnn::Mat &inputMat, const std::uint8_t *bgra_data)
{
	const float32x4_t v_norm = vdupq_n_f32(1.0f / 255.0f);

	float *r_channel = inputMat.channel(0);
	float *g_channel = inputMat.channel(1);
	float *b_channel = inputMat.channel(2);

	for (int i = 0; i < SelfieSegmenter::PIXEL_COUNT; i += 8) {
		uint8x8x4_t bgra_vec = vld4_u8(bgra_data + i * 4);

		uint16x8_t b_u16 = vmovl_u8(bgra_vec.val[0]);
		uint16x8_t g_u16 = vmovl_u8(bgra_vec.val[1]);
		uint16x8_t r_u16 = vmovl_u8(bgra_vec.val[2]);

		float32x4_t b_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(b_u16))), v_norm);
		float32x4_t g_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(g_u16))), v_norm);
		float32x4_t r_f32_low = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_low_u16(r_u16))), v_norm);

		float32x4_t b_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(b_u16))), v_norm);
		float32x4_t g_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(g_u16))), v_norm);
		float32x4_t r_f32_high = vmulq_f32(vcvtq_f32_u32(vmovl_u16(vget_high_u16(r_u16))), v_norm);

		vst1q_f32(b_channel + i, b_f32_low);
		vst1q_f32(b_channel + i + 4, b_f32_high);
		vst1q_f32(g_channel + i, g_f32_low);
		vst1q_f32(g_channel + i + 4, g_f32_high);
		vst1q_f32(r_channel + i, r_f32_low);
		vst1q_f32(r_channel + i + 4, r_f32_high);
	}
}

#endif // SELFIE_SEGMENTER_HAVE_NEON

#ifdef SELFIE_SEGMENTER_CHECK_AVX2

#if !defined(_MSC_VER)
__attribute__((target("xsave")))
#endif
inline bool
checkIfAVX2Available()
{
	int cpuInfo[4];
	auto cpuid = [&](int leaf, int subleaf = 0) {
#if defined(_MSC_VER)
		__cpuidex(cpuInfo, leaf, subleaf);
#else
		__cpuid_count(leaf, subleaf, cpuInfo[0], cpuInfo[1], cpuInfo[2], cpuInfo[3]);
#endif
	};

	cpuid(1);
	bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
	if (!osxsave)
		return false;

	unsigned long long xcr_val = _xgetbv(0);
	if ((xcr_val & 0x6) != 0x6)
		return false;

	cpuid(7, 0);
	return (cpuInfo[1] & (1 << 5)) != 0;
}

#if !defined(_MSC_VER)
__attribute__((target("avx,avx2")))
#endif
inline void
copyDataToMatAVX2(ncnn::Mat &inputMat, const std::uint8_t *bgra_data)
{
	float *r_channel = inputMat.channel(0);
	float *g_channel = inputMat.channel(1);
	float *b_channel = inputMat.channel(2);

	constexpr int PIXELS_PER_LOOP = 8;
	const int num_loops = SelfieSegmenter::PIXEL_COUNT / PIXELS_PER_LOOP;

	const __m256 v_inv_255 = _mm256_set1_ps(1.0f / 255.0f);
	const __m256i mask_u8 = _mm256_set1_epi32(0x000000FF);

	for (int i = 0; i < num_loops; ++i) {
		const int offset = i * PIXELS_PER_LOOP;
		const int data_offset = offset * 4;

		__m256i bgra_u32 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(bgra_data + data_offset));
		__m256i b_u32 = _mm256_and_si256(bgra_u32, mask_u8);
		__m256i g_u32 = _mm256_and_si256(_mm256_srli_epi32(bgra_u32, 8), mask_u8);
		__m256i r_u32 = _mm256_and_si256(_mm256_srli_epi32(bgra_u32, 16), mask_u8);

		__m256 b_ps = _mm256_cvtepi32_ps(b_u32);
		__m256 g_ps = _mm256_cvtepi32_ps(g_u32);
		__m256 r_ps = _mm256_cvtepi32_ps(r_u32);

		b_ps = _mm256_mul_ps(b_ps, v_inv_255);
		g_ps = _mm256_mul_ps(g_ps, v_inv_255);
		r_ps = _mm256_mul_ps(r_ps, v_inv_255);

		_mm256_storeu_ps(b_channel + offset, b_ps);
		_mm256_storeu_ps(g_channel + offset, g_ps);
		_mm256_storeu_ps(r_channel + offset, r_ps);
	}
}

#else

inline bool checkIfAVX2Available()
{
	return false;
}

#endif // SELFIE_SEGMENTER_CHECK_AVX2

} // namespace

EfficientNet::EfficientNet(const ncnn::Net &_efficientNet, int numClasses)
	: efficientNet(_efficientNet),
	  outputBuffer(numClasses),
	  isAVX2Available(checkIfAVX2Available())
{
	inputMat.create(INPUT_WIDTH, INPUT_HEIGHT, 3, sizeof(float));
}

void EfficientNet::process(const std::uint8_t *bgra_data)
{
	if (!bgra_data) {
		return;
	}

	preprocess(bgra_data);

	ncnn::Extractor ex = efficientNet.create_extractor();
	ex.input("in0", inputMat);
	ex.extract("out0", outputMat);

	outputBuffer.write([this](std::vector<float> &maskToWrite) { postprocess(maskToWrite); });
}

const std::vector<float> &EfficientNet::getOutputBuffer() const
{
	return outputBuffer.read();
}

void EfficientNet::preprocess(const std::uint8_t *bgra_data)
{
	// #if defined(EFFICIENT_NET_HAVE_NEON)
	// 	copyDataToMatNeon(inputMat, bgra_data);
	// #elif defined(EFFICIENT_NET_CHECK_AVX2)
	// 	if (isAVX2Available) {
	// 		copyDataToMatAVX2(inputMat, bgra_data);
	// 	} else {
	// 		copyDataToMatNaive(inputMat, bgra_data);
	// 	}
	// #else
	// 	copyDataToMatNaive(inputMat, bgra_data);
	// #endif
	copyDataToMatNaive(inputMat, bgra_data);
}

void EfficientNet::postprocess(std::vector<float> &mask) const
{
	const float *src_ptr = outputMat.channel(0);
	for (std::size_t i = 0; i < mask.size(); i++) {
		mask[i] = src_ptr[i];
	}
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
