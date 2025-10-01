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
	: running(true)
{
	serverThread = std::thread([this]() {
		uWS::App()
			.ws<UserData>("/", {
				.open = [this](auto *ws) {
					std::lock_guard<std::mutex> lock(mutex);
					clients.insert(ws);
				},
				.close = [this](auto *ws, int, std::string_view) {
					std::lock_guard<std::mutex> lock(mutex);
					clients.erase(ws);
				},
			})
			.listen(54834, [](auto *token) {
				if (!token) {
					// Handle listen error
				}
			})
			.run();
	});
}

WebSocketServer::~WebSocketServer()
{
	running = false;
	if (serverThread.joinable()) serverThread.join();
}

void WebSocketServer::broadcast(const std::string &message)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto *ws : clients) {
		ws->send(message, uWS::OpCode::TEXT);
	}
}

} // namespace LiveUniteTools
} // namespace KaitoTokyo
