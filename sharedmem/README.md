# Shared Memory — IPC Demo

> **Assigned to:** Brayan (UI Process member)

---

## What is Shared Memory?

**Shared memory** is the **fastest** form of IPC because processes communicate by reading and writing directly to the same region of physical memory — no data is copied through the kernel.

Key characteristics:
- **Zero-copy** — data goes directly from one process's address space to another.
- **Fastest IPC** — no kernel involvement per data transfer (only setup/teardown).
- **Named** — identified by `/pbl_shm`, accessible to any process with permission.
- **Requires synchronization** — since multiple processes share memory, a semaphore (or mutex) is essential to prevent race conditions.
- **Persistent** — the shared memory object survives until `shm_unlink()` is called.

---

## Semaphore Synchronization

Without synchronization, the reader might read stale or partially written data. This demo uses a **POSIX named semaphore** (`/pbl_sem`) to coordinate access:

| Operation | When | Effect |
|-----------|------|--------|
| `sem_post()` | Called by **writer** after each write | Increments semaphore → signals reader |
| `sem_wait()` | Called by **reader** before each read | Decrements semaphore → blocks if 0 |

The semaphore starts at **0**, so the reader always blocks until the writer signals it.

---

## Architecture Diagram

```
  ┌──────────────────────────────────────────────────────────────┐
  │                        Kernel                                │
  │                                                              │
  │   ┌─────────────────────────┐   ┌────────────────────────┐  │
  │   │  Shared Memory /pbl_shm │   │  Semaphore /pbl_sem    │  │
  │   │  ┌───────────────────┐  │   │                        │  │
  │   │  │ SharedData struct │  │   │   value: 0 (initial)   │  │
  │   │  │  int count        │  │   │                        │  │
  │   │  │  char message[256]│  │   └────────────────────────┘  │
  │   │  └───────────────────┘  │            ▲         │        │
  │   └────────────┬────────────┘            │         │        │
  │                │                    sem_post    sem_wait    │
  └────────────────┼─────────────────────────┼─────────┼────────┘
                   │                         │         │
          mmap / read-write             writer.c    reader.c
                   │                         │         │
         ┌─────────┴──┐                      │         │
         │            │                      │         │
      writer.c     reader.c                  └─────────┘
      (RDWR)       (RDONLY)               Synchronization
```

---

## Files

| File | Description |
|------|-------------|
| `writer.c` | Creates shared memory + semaphore, writes struct 5 times |
| `reader.c` | Opens shared memory + semaphore, reads struct 5 times |

---

## Shared Data Structure

Both programs use the same struct (must match exactly):

```c
typedef struct {
    int  count;          // Iteration number (1–5)
    char message[256];   // Text message from writer
} SharedData;
```

---

## How to Compile

```bash
# Compile writer
gcc writer.c -o writer -lrt -lpthread

# Compile reader
gcc reader.c -o reader -lrt -lpthread
```

> **`-lrt`** — POSIX real-time extensions (`shm_open`, `sem_open`, etc.)  
> **`-lpthread`** — POSIX threads library (required for semaphore functions on some systems)

---

## How to Run

**Start the reader first** (it blocks on `sem_wait`):

```bash
# Terminal 1
./reader
```

Then start the writer:

```bash
# Terminal 2
./writer
```

### Expected output

**Terminal 1 (reader):**
```
[Reader] Opened shared memory '/pbl_shm'.
[Reader] Waiting for 5 writes from writer...

[Reader] Read   [count=1]: "Hello from writer — iteration 1 (PID 1234)"
[Reader] Read   [count=2]: "Hello from writer — iteration 2 (PID 1234)"
...
[Reader] All reads complete. Exiting.
```

**Terminal 2 (writer):**
```
[Writer] Shared memory '/pbl_shm' created.
[Writer] Writing 5 messages (1-second intervals)...

[Writer] Wrote  [count=1]: "Hello from writer — iteration 1 (PID 1234)"
...
[Writer] Done. Shared memory and semaphore unlinked.
```

---

## Step-by-Step Flow

```
writer.c                           reader.c
   │                                   │
   │  shm_open(O_CREAT|O_RDWR)        │
   │  ftruncate(sizeof(SharedData))   │  shm_open(O_RDONLY) ← retries if not ready
   │  mmap(PROT_READ|PROT_WRITE)      │  mmap(PROT_READ)
   │                                   │
   │  [Write iteration 1]              │
   │  shm_ptr->count = 1               │  sem_wait() ◄─── blocks here
   │  shm_ptr->message = "..."         │
   │  sem_post() ──────────────────────┤
   │                                   │  sem_wait() returns
   │                                   │  [Read count=1, message="..."]
   │  [Write iteration 2]              │
   │  sem_post() ──────────────────────┤
   │                                   │  [Read count=2, ...]
   │       ...                         │       ...
   │  munmap / sem_close               │  munmap / sem_close
   │  shm_unlink / sem_unlink          │
```

---

## Key API Reference

| Function | Purpose |
|----------|---------|
| `shm_open(name, flags, mode)` | Create/open a POSIX shared memory object |
| `ftruncate(fd, size)` | Set the size of the shared memory |
| `mmap(NULL, size, prot, MAP_SHARED, fd, 0)` | Map shared memory into address space |
| `munmap(ptr, size)` | Unmap shared memory |
| `shm_unlink(name)` | Remove the shared memory object |
| `sem_open(name, flags, mode, value)` | Create/open a named semaphore |
| `sem_post(sem)` | Increment semaphore (signal) |
| `sem_wait(sem)` | Decrement semaphore (wait/block if 0) |
| `sem_close(sem)` | Close the semaphore descriptor |
| `sem_unlink(name)` | Remove the named semaphore |
