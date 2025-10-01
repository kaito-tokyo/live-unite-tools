#pragma once

#include "IWebSocketSender.hpp"

class WebSocketSenderImpl : public IWebSocketSender {
public:
	WebSocketSenderImpl(int port = 9001);
	void send(const std::string &message) override;
};
