#include "WebSocketSender.hpp"
#include <uwebsockets/App.h>
#include <mutex>
#include <set>
#include <thread>

class WebSocketSenderImpl::Impl {
public:
	Impl(int port)
	{
		std::thread([this, port]() {
			uWS::App().ws<PerSocketData>("/*", {
				.open = [this](auto *ws) {
					std::lock_guard<std::mutex> lock(mutex);
					connections.insert(ws);
				},
				.close = [this](auto *ws, int, std::string_view) {
					std::lock_guard<std::mutex> lock(mutex);
					connections.erase(ws);
				}
			}).listen(port, nullptr).run();
		}).detach();
	}

	void send(const std::string &message)
	{
		std::lock_guard<std::mutex> lock(mutex);
		for (auto *ws : connections) {
			ws->send(message, uWS::OpCode::TEXT);
		}
	}

private:
	struct PerSocketData {};
	std::set<uWS::WebSocket<false, true, PerSocketData> *> connections;
	std::mutex mutex;
};

WebSocketSenderImpl::WebSocketSenderImpl(int port)
	: pImpl(new Impl(port))
{
}

void WebSocketSenderImpl::send(const std::string &message)
{
	pImpl->send(message);
}
