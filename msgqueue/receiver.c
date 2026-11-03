/*
 * receiver.c — IPC Demo: POSIX Message Queues (Receiver)
 * --------------------------------------------------------
 * Opens /pbl_msgq, receives 5 messages, then unlinks the queue.
 *
 * Compile: gcc receiver.c -o receiver -lrt
 * Run first, then run sender: ./receiver
 *
 * Author: Keith (Logging Process member)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#define QUEUE_NAME  "/pbl_msgq"
#define MAX_SIZE    256
#define NUM_MSGS    5

int main(void)
{
    mqd_t mq;
    char  buffer[MAX_SIZE + 1];
    unsigned int prio;

    struct mq_attr attr = {
        .mq_flags   = 0,
        .mq_maxmsg  = 10,
        .mq_msgsize = MAX_SIZE,
        .mq_curmsgs = 0
    };

    /* ── Open (or create) the message queue in read mode ─────────── */
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("[Receiver] Opened message queue: %s\n", QUEUE_NAME);
    printf("[Receiver] Waiting for %d messages...\n\n", NUM_MSGS);

    /* ── Receive NUM_MSGS messages ────────────────────────────────── */
    for (int i = 0; i < NUM_MSGS; i++) {
        ssize_t bytes_read = mq_receive(mq, buffer, MAX_SIZE, &prio);
        if (bytes_read == -1) {
            perror("mq_receive");
            mq_close(mq);
            mq_unlink(QUEUE_NAME);
            exit(EXIT_FAILURE);
        }
        buffer[bytes_read] = '\0';
        printf("[Receiver] Received (prio=%u): \"%s\"\n", prio, buffer);
    }

    /* ── Clean up ─────────────────────────────────────────────────── */
    mq_close(mq);

    if (mq_unlink(QUEUE_NAME) == -1) {
        perror("mq_unlink");
        exit(EXIT_FAILURE);
    }

    printf("\n[Receiver] All messages received. Queue '%s' unlinked.\n",
           QUEUE_NAME);

    return 0;
}
