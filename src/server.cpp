#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "gpio_controller.hpp"

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        std::perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "IoT Gateway Server active on port 8080...\n";

    try {
        aeroswarm::GpioController gpio;

        while (true) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                std::perror("accept");
                continue;
            }

            char buffer[1024] = {0};
            read(new_socket, buffer, 1024);
            std::string req(buffer);

            std::string response;
            if (req.find("GET /api/status") != std::string::npos) {
                int state = gpio.read_pins();
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"active\",\"pins\":" + std::to_string(state) + "}\n";
            } else if (req.find("POST /api/relay") != std::string::npos) {
                if (req.find("Authorization: Bearer Natska2026") != std::string::npos) {
                    response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\":true,\"relay\":1,\"state\":\"HIGH\"}\n";
                } else {
                    response = "HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Unauthorized\"}\n";
                }
            } else {
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Not Found\"}\n";
            }

            write(new_socket, response.c_str(), response.length());
            close(new_socket);
        }
    } catch (const std::exception& e) {
        std::cerr << "Server Error: " << e.what() << "\n";
    }

    return 0;
}
