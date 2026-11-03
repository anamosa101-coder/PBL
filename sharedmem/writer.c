/*
 * writer.c — IPC Demo: POSIX Shared Memory (Writer)
 * ---------------------------------------------------
 * Creates shared memory /pbl_shm and a named semaphore /pbl_sem.
 * Writes a SharedData struct 5 times (1-second intervals),
 * signalling the reader after each write via sem_post().
 *
 * Compile: gcc writer.c -o writer -lrt -lpthread
 * Run (reader first, then writer): ./writer
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

#define SHM_NAME    "/pbl_shm"
#define SEM_NAME    "/pbl_sem"
#define NUM_WRITES  5

/* Shared data structure */
typedef struct {
    int  count;
    char message[256];
} SharedData;

int main(void)
{
    int      shm_fd;
    SharedData *shm_ptr;
    sem_t   *sem;

    /* ── Open / create the semaphore (initial value = 0) ─────────── */
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    /* ── Create the shared memory object ─────────────────────────── */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("shm_open");
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    /* ── Size the shared memory to hold one SharedData struct ──────── */
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    /* ── Map shared memory into this process's address space ─────── */
    shm_ptr = mmap(NULL, sizeof(SharedData),
                   PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        sem_close(sem);
        exit(EXIT_FAILURE);
    }
    close(shm_fd);   /* fd no longer needed after mmap */

    printf("[Writer] Shared memory '%s' created.\n", SHM_NAME);
    printf("[Writer] Writing %d messages (1-second intervals)...\n\n",
           NUM_WRITES);

    /* ── Write loop ───────────────────────────────────────────────── */
    for (int i = 1; i <= NUM_WRITES; i++) {
        shm_ptr->count = i;
        snprintf(shm_ptr->message, sizeof(shm_ptr->message),
                 "Hello from writer — iteration %d (PID %d)", i, getpid());

        printf("[Writer] Wrote  [count=%d]: \"%s\"\n",
               shm_ptr->count, shm_ptr->message);

        /* Signal reader that new data is available */
        sem_post(sem);

        sleep(1);
    }

    /* ── Clean up ─────────────────────────────────────────────────── */
    munmap(shm_ptr, sizeof(SharedData));
    sem_close(sem);

    /* Unlink shared memory and semaphore — reader should be done by now */
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);

    printf("\n[Writer] Done. Shared memory and semaphore unlinked.\n");

    return 0;
}
