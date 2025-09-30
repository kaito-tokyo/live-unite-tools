/*
Bridge Utils
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <mutex>
#include <sstream>
#include <string_view>

#include <util/base.h>

#include "ILogger.hpp"

namespace KaitoTokyo {
namespace BridgeUtils {

class ObsLogger final : public ILogger {
public:
	ObsLogger(const char *_prefix) : prefix(_prefix) {}

protected:
	static constexpr size_t MAX_LOG_CHUNK_SIZE = 4000;

	void log(LogLevel level, std::string_view message) const noexcept override
	{
		std::lock_guard<std::mutex> lock(mtx);
		int blogLevel;
		switch (level) {
		case LogLevel::Debug:
			blogLevel = LOG_DEBUG;
			break;
		case LogLevel::Info:
			blogLevel = LOG_INFO;
			break;
		case LogLevel::Warn:
			blogLevel = LOG_WARNING;
			break;
		case LogLevel::Error:
			blogLevel = LOG_ERROR;
			break;
		default:
			blog(LOG_ERROR, "[LOGGER FATAL] Unknown log level: %d\n", static_cast<int>(level));
			return;
		}

		if (message.length() <= MAX_LOG_CHUNK_SIZE) {
			blog(blogLevel, "%.*s", static_cast<int>(message.length()), message.data());
		} else {
			for (size_t i = 0; i < message.length(); i += MAX_LOG_CHUNK_SIZE) {
				const auto chunk = message.substr(i, MAX_LOG_CHUNK_SIZE);
				blog(blogLevel, "%.*s", static_cast<int>(chunk.length()), chunk.data());
			}
		}
	}

protected:
	const char *getPrefix() const noexcept override { return prefix; }

private:
	const char *prefix;
	mutable std::mutex mtx;
};

} // namespace BridgeUtils
} // namespace KaitoTokyo
