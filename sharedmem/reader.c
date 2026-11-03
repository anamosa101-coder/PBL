/*
 * reader.c — IPC Demo: POSIX Shared Memory (Reader)
 * ---------------------------------------------------
 * Opens shared memory /pbl_shm and semaphore /pbl_sem.
 * Uses sem_wait() to block until writer signals, then reads
 * and prints the SharedData struct — repeated 5 times.
 *
 * Compile: gcc reader.c -o reader -lrt -lpthread
 * Run first, then writer: ./reader
 *
 * Author: Brayan (UI Process member)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHM_NAME   "/pbl_shm"
#define SEM_NAME   "/pbl_sem"
#define NUM_READS  5

/* Must match the struct in writer.c */
typedef struct {
    int  count;
    char message[256];
} SharedData;

int main(void)
{
    int      shm_fd;
    SharedData *shm_ptr;
    sem_t   *sem;

    /* ── Open the semaphore (must already exist or be created here) ─ */
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    /* ── Open the shared memory object (created by writer) ───────── */
    /* Retry briefly in case writer hasn't created it yet */
    int retries = 10;
    do {
        shm_fd = shm_open(SHM_NAME, O_RDONLY, 0644);
        if (shm_fd != -1) break;
        printf("[Reader] Waiting for writer to create shared memory...\n");
        sleep(1);
    } while (--retries > 0);

    if (shm_fd == -1) {
        perror("shm_open");
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    /* ── Map shared memory into this process's address space ─────── */
    shm_ptr = mmap(NULL, sizeof(SharedData),
                   PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        sem_close(sem);
        exit(EXIT_FAILURE);
    }
    close(shm_fd);

    printf("[Reader] Opened shared memory '%s'.\n", SHM_NAME);
    printf("[Reader] Waiting for %d writes from writer...\n\n", NUM_READS);

    /* ── Read loop ────────────────────────────────────────────────── */
    for (int i = 0; i < NUM_READS; i++) {
        /* Block until writer calls sem_post() */
        if (sem_wait(sem) == -1) {
            perror("sem_wait");
            munmap(shm_ptr, sizeof(SharedData));
            sem_close(sem);
            exit(EXIT_FAILURE);
        }

        printf("[Reader] Read   [count=%d]: \"%s\"\n",
               shm_ptr->count, shm_ptr->message);
    }

    /* ── Clean up ─────────────────────────────────────────────────── */
    munmap(shm_ptr, sizeof(SharedData));
    sem_close(sem);

    printf("\n[Reader] All reads complete. Exiting.\n");

    return 0;
}
