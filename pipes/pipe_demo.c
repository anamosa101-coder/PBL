/*
 * pipe_demo.c — IPC Demo: Anonymous Pipes
 * ----------------------------------------
 * Demonstrates one-way communication between a parent
 * and child process using an anonymous pipe (pipe(2)).
 *
 * Pipe direction:
 *   Parent (writer) ──[pipe]──► Child (reader)
 *
 * Compile: gcc pipe_demo.c -o pipe_demo
 * Run:     ./pipe_demo
 *
 * Author: Anam (Team Lead)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_MESSAGES 5
#define MSG_SIZE     64

int main(void)
{
    int pipefd[2];   /* pipefd[0] = read end, pipefd[1] = write end */
    pid_t pid;
    char buffer[MSG_SIZE];

    /* ── Create the pipe ─────────────────────────────────────────── */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* ── Fork ────────────────────────────────────────────────────── */
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /* ══ CHILD PROCESS — READER ══════════════════════════════ */
        close(pipefd[1]);   /* child does not write; close write end */

        printf("[Child  PID %d] Waiting to read from pipe...\n\n", getpid());

        for (int i = 0; i < NUM_MESSAGES; i++) {
            ssize_t bytes = read(pipefd[0], buffer, MSG_SIZE);
            if (bytes == -1) {
                perror("read");
                close(pipefd[0]);
                exit(EXIT_FAILURE);
            }
            if (bytes == 0) {
                /* Write end closed before we expected — pipe broken */
                fprintf(stderr, "[Child] Pipe closed unexpectedly.\n");
                break;
            }
            buffer[bytes] = '\0';
            printf("[Child  PID %d] Received: \"%s\"\n", getpid(), buffer);
        }

        close(pipefd[0]);
        printf("\n[Child  PID %d] Done reading. Exiting.\n", getpid());
        exit(EXIT_SUCCESS);

    } else {
        /* ══ PARENT PROCESS — WRITER ══════════════════════════════ */
        close(pipefd[0]);   /* parent does not read; close read end */

        printf("[Parent PID %d] Sending %d messages to child (PID %d)...\n\n",
               getpid(), NUM_MESSAGES, pid);

        for (int i = 1; i <= NUM_MESSAGES; i++) {
            snprintf(buffer, MSG_SIZE, "Message %d", i);
            ssize_t bytes = write(pipefd[1], buffer, strlen(buffer));
            if (bytes == -1) {
                perror("write");
                close(pipefd[1]);
                exit(EXIT_FAILURE);
            }
            printf("[Parent PID %d] Sent:     \"%s\"\n", getpid(), buffer);
            sleep(1);   /* 1-second pause so output is easy to follow */
        }

        close(pipefd[1]);   /* close write end → EOF signal to child */
        printf("\n[Parent PID %d] All messages sent. Waiting for child...\n",
               getpid());

        /* Wait for the child to finish */
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status))
            printf("[Parent PID %d] Child exited with status %d.\n",
                   getpid(), WEXITSTATUS(status));

        printf("[Parent PID %d] Demo complete.\n", getpid());
    }

    return 0;
}
