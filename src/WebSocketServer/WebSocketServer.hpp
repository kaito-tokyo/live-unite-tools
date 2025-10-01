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

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>

#include <uwebsockets/App.h>
#include <uwebsockets/Loop.h>

namespace KaitoTokyo {
namespace LiveUniteTools {

class WebSocketServer {
public:
    static std::shared_ptr<WebSocketServer> getSharedWebSocketServer();

    WebSocketServer();
    ~WebSocketServer();

    void broadcast(const std::string &message);

private:
    struct UserData {};

    uWS::Loop* loop = nullptr;
    struct us_listen_socket_t *listenSocket = nullptr;

    std::unordered_set<uWS::WebSocket<false, true, UserData> *> clients;
    std::mutex clientsMutex;
    std::thread serverThread;
    std::atomic<bool> running;
};

} // namespace LiveUniteTools
} // namespace KaitoTokyo
