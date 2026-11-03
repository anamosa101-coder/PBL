/*
 * client.c — IPC Demo: Unix Domain Sockets (Client)
 * ---------------------------------------------------
 * Connects to the server at /tmp/pbl_socket.
 * Sends 5 messages and prints each echo reply.
 *
 * Compile: gcc client.c -o client
 * Run after server: ./client
 *
 * Author: Atul (Core Process member)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH  "/tmp/pbl_socket"
#define BUFFER_SIZE  256
#define NUM_MSGS     5

int main(void)
{
    int sockfd;
    struct sockaddr_un addr;
    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];

    /* ── Create Unix domain stream socket ────────────────────────── */
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    /* ── Connect to server ───────────────────────────────────────── */
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect (is the server running?)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[Client] Connected to server at %s\n\n", SOCKET_PATH);

    /* ── Send-and-receive loop ────────────────────────────────────── */
    for (int i = 1; i <= NUM_MSGS; i++) {
        snprintf(send_buf, BUFFER_SIZE, "Hello from client — message %d", i);

        if (send(sockfd, send_buf, strlen(send_buf), 0) == -1) {
            perror("send");
            break;
        }
        printf("[Client] Sent:     \"%s\"\n", send_buf);

        /* Wait for echo reply */
        ssize_t bytes = recv(sockfd, recv_buf, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            if (bytes == 0)
                printf("[Client] Server closed connection.\n");
            else
                perror("recv");
            break;
        }
        recv_buf[bytes] = '\0';
        printf("[Client] Received: \"%s\"\n\n", recv_buf);

        sleep(1);
    }

    /* ── Clean up ─────────────────────────────────────────────────── */
    close(sockfd);
    printf("[Client] Done. Connection closed.\n");

    return 0;
}
