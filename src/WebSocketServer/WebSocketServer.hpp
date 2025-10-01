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

#include <uwebsockets/App.h>

namespace KaitoTokyo {
namespace LiveUniteTools {

class WebSocketServer {
    void start() {
        const int port = 57491;
        uWS::App()
        .ws<void>("/*", {
            .open = [](auto *ws) {
                /* Open event */
            },
            .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                /* Message event */
            },
            .close = [](auto *ws, int code, std::string_view message) {
                /* Close event */
            }
        })
        .listen(port, [port](auto* listenSocket) {
            if (listenSocket) {
                std::cout << "Listening on port " << port << std::endl;
            } else {
                std::cout << "Failed to listen on port " << port << std::endl;
            }
        })
        .run();
    }
}

}
}
