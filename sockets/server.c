/*
 * server.c — IPC Demo: Unix Domain Sockets (Server)
 * ---------------------------------------------------
 * Creates a Unix domain socket at /tmp/pbl_socket.
 * Accepts one client connection, receives 5 messages,
 * and echoes each back with "[ECHO]: " prefix.
 *
 * Compile: gcc server.c -o server
 * Run first: ./server
 *
 * Author: Atul (Core Process member)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define SOCKET_PATH  "/tmp/pbl_socket"
#define BUFFER_SIZE  256
#define NUM_MSGS     5

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char   buffer[BUFFER_SIZE];
    char   echo_buf[BUFFER_SIZE + 8];  /* extra space for "[ECHO]: " */

    /* ── Create Unix domain stream socket ────────────────────────── */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* ── Remove stale socket file if it exists ───────────────────── */
    unlink(SOCKET_PATH);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    /* ── Bind socket to filesystem path ──────────────────────────── */
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* ── Listen for one incoming connection ──────────────────────── */
    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    printf("[Server] Listening on %s\n", SOCKET_PATH);
    printf("[Server] Waiting for client...\n\n");

    /* ── Accept client connection ─────────────────────────────────── */
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }

    printf("[Server] Client connected.\n\n");

    /* ── Receive-and-echo loop ────────────────────────────────────── */
    for (int i = 0; i < NUM_MSGS; i++) {
        ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            if (bytes == 0)
                printf("[Server] Client disconnected early.\n");
            else
                perror("recv");
            break;
        }
        buffer[bytes] = '\0';
        printf("[Server] Received: \"%s\"\n", buffer);

        /* Prepend "[ECHO]: " and send back */
        snprintf(echo_buf, sizeof(echo_buf), "[ECHO]: %s", buffer);
        if (send(client_fd, echo_buf, strlen(echo_buf), 0) == -1) {
            perror("send");
            break;
        }
        printf("[Server] Echoed:   \"%s\"\n\n", echo_buf);
    }

    /* ── Clean up ─────────────────────────────────────────────────── */
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    printf("[Server] Session complete. Socket removed.\n");

    return 0;
}
