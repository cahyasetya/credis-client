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
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 64
#define SERVER_PORT 1234
#define SERVER_IP "127.0.0.1"
#define MAX_MESSAGE_SIZE 4096

// Function prototypes
struct sockaddr_in create_server_addr(const char* ip, uint16_t port);
int connect_to_server(const char* ip, uint16_t port);
int send_message(int sock_fd, const char* message);
int receive_message(int sock_fd, char* buffer, size_t buffer_size);
static int32_t query(int fd, const char *text);
int32_t read_full(int fd, char *buf, size_t n);
int32_t write_full(int fd, const char *buf, size_t n);

int main() {
    int sock_fd = connect_to_server(SERVER_IP, SERVER_PORT);
    if (sock_fd < 0) {
        return 1;
    }

    const char* message = "hello";
    if (query(sock_fd, message) < 0) {
        close(sock_fd);
        return 1;
    }

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

static int32_t query(int fd, const char *text) {
    uint32_t len = strlen(text);
    if (len > MAX_MESSAGE_SIZE) {
        logger("ERROR", "Message too long");
        return -1;
    }

    char wbuf[4 + MAX_MESSAGE_SIZE];
    memcpy(wbuf, &len, 4);
    memcpy(wbuf + 4, text, len);

    if (write_full(fd, wbuf, 4 + len) != 0) {
        logger("ERROR", "Write error: %s", strerror(errno));
        return -1;
    }

    char rbuf[4 + MAX_MESSAGE_SIZE + 1];
    if (read_full(fd, rbuf, 4) != 0) {
        logger("ERROR", "Read error: %s", strerror(errno) ?: "Read nothing");
        return -1;
    }

    memcpy(&len, rbuf, 4);
    if (len > MAX_MESSAGE_SIZE) {
        logger("ERROR", "Response too long: %u", len);
        return -1;
    }

    if (read_full(fd, rbuf + 4, len) != 0) {
        logger("ERROR", "Read error: %s", strerror(errno));
        return -1;
    }

    rbuf[4 + len] = '\0';
    logger("INFO", "Received: %s", rbuf + 4);
    return 0;
}
