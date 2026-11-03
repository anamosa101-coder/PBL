/*
 * sender.c — IPC Demo: POSIX Message Queues (Sender)
 * ----------------------------------------------------
 * Opens /pbl_msgq and sends 5 typed messages.
 *
 * Compile: gcc sender.c -o sender -lrt
 * Run (after starting receiver): ./sender
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
#define MSG_PRIO    1       /* message priority (0 = lowest) */

int main(void)
{
    mqd_t mq;

    struct mq_attr attr = {
        .mq_flags   = 0,
        .mq_maxmsg  = 10,       /* max messages in queue */
        .mq_msgsize = MAX_SIZE, /* max size of one message */
        .mq_curmsgs = 0
    };

    /* ── Open (or create) the message queue ──────────────────────── */
    mq = mq_open(QUEUE_NAME, O_CREAT | O_WRONLY, 0644, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    printf("[Sender] Opened message queue: %s\n\n", QUEUE_NAME);

    /* ── Send NUM_MSGS messages ───────────────────────────────────── */
    for (int i = 1; i <= NUM_MSGS; i++) {
        char msg[MAX_SIZE];
        snprintf(msg, MAX_SIZE, "Message %d from sender (PID %d)", i, getpid());

        if (mq_send(mq, msg, strlen(msg) + 1, MSG_PRIO) == -1) {
            perror("mq_send");
            mq_close(mq);
            exit(EXIT_FAILURE);
        }

        printf("[Sender] Sent (prio=%d): \"%s\"\n", MSG_PRIO, msg);
        sleep(1);
    }

    /* ── Clean up ─────────────────────────────────────────────────── */
    mq_close(mq);
    printf("\n[Sender] All messages sent. Queue still open for receiver.\n");

    return 0;
}
