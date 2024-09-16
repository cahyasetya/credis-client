#include "../credis-common/include/credis_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdarg.h>

#define BUFFER_SIZE 64
#define SERVER_PORT 1234
#define SERVER_IP "127.0.0.1"

// Function prototypes
struct sockaddr_in create_server_addr(const char* ip, uint16_t port);
int connect_to_server(const char* ip, uint16_t port);
int send_message(int sock_fd, const char* message);
int receive_message(int sock_fd, char* buffer, size_t buffer_size);

int main() {
    int sock_fd = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock_fd < 0) {
        return 1;
    }

    const char* message = "hello";
    if (send_message(sock_fd, message) < 0) {
        close(sock_fd);
        return 1;
    }

    char response[BUFFER_SIZE];
    if (receive_message(sock_fd, response, sizeof(response)) < 0) {
        close(sock_fd);
        return 1;
    }

    logger("INFO", "Received: %s", response);

    close(sock_fd);
    return 0;
}

struct sockaddr_in create_server_addr(const char* ip, uint16_t port) {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    return addr;
}

int connect_to_server(const char* ip, uint16_t port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        logger("ERROR", "Error creating socket: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in server_addr = create_server_addr(ip, port);
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        logger("ERROR", "Error connecting to server: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    logger("INFO", "Connected to server successfully");
    return sock_fd;
}

int send_message(int sock_fd, const char* message) {
    ssize_t bytes_sent = send(sock_fd, message, strlen(message), 0);
    if (bytes_sent < 0) {
        logger("ERROR", "Error sending message: %s", strerror(errno));
        return -1;
    }
    logger("INFO", "Sent message: %s", message);
    return 0;
}

int receive_message(int sock_fd, char* buffer, size_t buffer_size) {
    ssize_t bytes_received = recv(sock_fd, buffer, buffer_size - 1, 0);
    if (bytes_received < 0) {
        logger("ERROR", "Error reading from socket: %s", strerror(errno));
        return -1;
    }
    buffer[bytes_received] = '\0';  // Null-terminate the received data
    return 0;
}
