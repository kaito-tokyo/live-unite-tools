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

#pragma once

#include <atomic>
#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <string>
#include <algorithm>

#include <net.h>

namespace KaitoTokyo {
namespace LiveUniteTools {
namespace EfficientNetDetail {

/**
 * @class OutputBuffer
 * @brief A thread-safe double buffer for managing output data.
 */

template<typename ItemType> class OutputBuffer {
private:
	std::array<std::vector<ItemType>, 2> buffers;
	std::atomic<int> readableIndex;
	mutable std::mutex bufferMutex;

public:
	explicit OutputBuffer(std::size_t size)
		: buffers{std::vector<ItemType>(size), std::vector<ItemType>(size)},
		  readableIndex(0)
	{
	}

	void write(std::function<void(std::vector<ItemType> &)> writeFunc)
	{
		std::lock_guard<std::mutex> lock(bufferMutex);
		const int writeIndex = (readableIndex.load(std::memory_order_relaxed) + 1) % 2;
		writeFunc(buffers[writeIndex]);
		readableIndex.store(writeIndex, std::memory_order_release);
	}

	const std::vector<ItemType> &read() const
	{
		// Lock-free read
		return buffers[readableIndex.load(std::memory_order_acquire)];
	}
};

} // namespace EfficientNetDetail

/**
 * @class EfficientNet
 * @brief A class that orchestrates image preprocessing, inference, and postprocessing.
 */
class EfficientNet {
public:
	static constexpr int INPUT_WIDTH = 224;
	static constexpr int INPUT_HEIGHT = 224;
	static constexpr int PIXEL_COUNT = INPUT_WIDTH * INPUT_HEIGHT;

private:
	const ncnn::Net &efficientNet;
	EfficientNetDetail::OutputBuffer<float> outputBuffer;
	ncnn::Mat inputMat;
	ncnn::Mat outputMat;
	bool isAVX2Available;

public:
	EfficientNet(const ncnn::Net &_efficientNet, int numClasses);
	void process(const std::uint8_t *bgra_data);
	const std::vector<float> &getOutputBuffer() const;

private:
	void preprocess(const std::uint8_t *bgra_data);
	void postprocess(std::vector<float> &mask) const;
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo
