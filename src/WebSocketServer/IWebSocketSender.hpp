#pragma once

#include <string>

class IWebSocketSender {
public:
	virtual ~IWebSocketSender() = default;
	virtual void send(const std::string &message) = 0;
};
