#include "../credis-common/include/credis_common.h"
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 1234
#define MAX_MESSAGE_SIZE 4096

// Function prototypes
sockaddr_in create_server_addr(const char* ip, uint16_t port);
int connect_to_server(const char* ip, uint16_t port);
int32_t query(int sock_fd, const char* request);

int main() {
    int sock_fd = connect_to_server(SERVER_IP, PORT);
    if (sock_fd < 0) {
        logger("ERROR", "Failed to connect to server");
        return 1;
    }

    const char* queries[] = {"hello1", "hello2", "hello3"};
    for (int i = 0; i < 3; ++i) {
        if (query(sock_fd, queries[i]) < 0) {
            close(sock_fd);
            logger("ERROR", "Query failed: %s", queries[i]);
            return 1;
        }
    }

    close(sock_fd);
    return 0;
}

sockaddr_in create_server_addr(const char* ip, uint16_t port) {
    sockaddr_in addr = {};
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

    sockaddr_in server_addr = create_server_addr(ip, port);
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        logger("ERROR", "Error connecting to server: %s", strerror(errno));
        close(sock_fd);
        return -1;
    }

    logger("INFO", "Connected to server successfully");
    return sock_fd;
}

int32_t query(int sock_fd, const char* request) {
    uint32_t len = strlen(request);
    if (len > MAX_MESSAGE_SIZE) {
        logger("ERROR", "Message too long");
        return -1;
    }

    char wbuf[4 + MAX_MESSAGE_SIZE];
    memcpy(wbuf, &len, 4);
    memcpy(wbuf + 4, request, len);

    if (write_full(sock_fd, wbuf, 4 + len) != 0) {
        logger("ERROR", "Write error: %s", strerror(errno));
        return -1;
    }

    char rbuf[4 + MAX_MESSAGE_SIZE + 1];
    if (read_full(sock_fd, rbuf, 4) != 0) {
        logger("ERROR", "Read error: %s", strerror(errno) ? strerror(errno) : "Read nothing");
        return -1;
    }

    memcpy(&len, rbuf, 4);
    if (len > MAX_MESSAGE_SIZE) {
        logger("ERROR", "Response too long: %u", len);
        return -1;
    }

    if (read_full(sock_fd, rbuf + 4, len) != 0) {
        logger("ERROR", "Read error: %s", strerror(errno));
        return -1;
    }

    rbuf[4 + len] = '\0';
    logger("INFO", "Received: %s", rbuf + 4);
    return 0;
}
