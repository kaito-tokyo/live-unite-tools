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

#include "WebSocketServer.hpp"

#include <future>

namespace KaitoTokyo {
namespace LiveUniteTools {

std::shared_ptr<WebSocketServer> WebSocketServer::getSharedWebSocketServer()
{
	static std::mutex mtx;
	static std::shared_ptr<WebSocketServer> instance;
	std::lock_guard<std::mutex> lock(mtx);
	if (!instance) {
		instance = std::make_shared<WebSocketServer>();
	}
	return instance;
}

WebSocketServer::WebSocketServer()
{
	std::promise<uWS::Loop *> loopPromise;
	auto loopFuture = loopPromise.get_future();

	serverThread = std::thread([this, p = std::move(loopPromise)]() mutable {
		uWS::Loop *threadLoop = uWS::Loop::get();
		p.set_value(threadLoop);

		uWS::TemplatedApp<false>::WebSocketBehavior<UserData> behavior;
		behavior.open = [this](auto *ws) {
			std::lock_guard<std::mutex> lock(clientsMutex);
			clients.insert(ws);
		};
		behavior.close = [this](auto *ws, int, std::string_view) {
			std::lock_guard<std::mutex> lock(clientsMutex);
			clients.erase(ws);
		};

		uWS::App()
			.ws<UserData>("/", std::move(behavior))
			.listen(54834,
				[this](auto *token) {
					if (token) {
						this->listenSocket = token;
						this->running = true;
					} else {
						this->running = false;
					}
				})
			.run();
	});

	loop = loopFuture.get();
}

WebSocketServer::~WebSocketServer()
{
	if (running.exchange(false)) {
		if (loop) {
			loop->defer([this]() {
				if (listenSocket) {
					us_listen_socket_close(0, listenSocket);
				}
				std::lock_guard<std::mutex> lock(clientsMutex);
				for (auto *ws : clients) {
					ws->end(1001, "Server shutting down");
				}
				clients.clear();
			});
		}
	}

	if (serverThread.joinable()) {
		serverThread.join();
	}
}

void WebSocketServer::broadcast(const std::string &message)
{
	if (!running || !loop)
		return;

	auto messageCopy = std::make_shared<std::string>(message);

	loop->defer([this, messageCopy]() {
		std::lock_guard<std::mutex> lock(clientsMutex);
		for (auto *ws : clients) {
			ws->send(*messageCopy, uWS::OpCode::TEXT);
		}
	});
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
