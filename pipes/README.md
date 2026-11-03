# Pipes — IPC Demo

> **Assigned to:** Anam (Team Lead)

---

## What are Pipes?

A **pipe** is one of the simplest forms of Inter-Process Communication (IPC). It provides a **one-way, unidirectional** data channel between two **related** processes (a parent and its forked child).

Key characteristics:
- **Unidirectional** — data flows in one direction only (writer → reader).
- **Anonymous** — no name in the filesystem; shared only via `fork()`.
- **In-kernel buffer** — the kernel manages the data buffer (typically 64 KB).
- **Related processes only** — both ends of the pipe must be in the same process family.
- **Blocking I/O** — `read()` blocks until data is available; `write()` blocks if the buffer is full.

```
  Parent Process           Kernel Pipe Buffer          Child Process
  ┌─────────────┐         ┌──────────────────┐        ┌─────────────┐
  │             │  write  │                  │  read  │             │
  │  (Writer)   │────────►│   [data queue]   │───────►│  (Reader)   │
  │             │         │                  │        │             │
  └─────────────┘         └──────────────────┘        └─────────────┘
   pipefd[1]                                            pipefd[0]
```

---

## Files

| File | Description |
|------|-------------|
| `pipe_demo.c` | Single-file demo — parent writes, child reads |

---

## How to Compile

```bash
gcc pipe_demo.c -o pipe_demo
```

No extra flags needed — pipes are part of the standard POSIX API in `<unistd.h>`.

---

## How to Run

```bash
./pipe_demo
```

Expected output (PIDs will differ):

```
[Parent PID 1234] Sending 5 messages to child (PID 1235)...

[Parent PID 1234] Sent:     "Message 1"
[Child  PID 1235] Received: "Message 1"
[Parent PID 1234] Sent:     "Message 2"
[Child  PID 1235] Received: "Message 2"
...
[Parent PID 1234] All messages sent. Waiting for child...
[Child  PID 1235] Done reading. Exiting.
[Parent PID 1234] Child exited with status 0.
[Parent PID 1234] Demo complete.
```

---

## How It Works — Architecture

### Step-by-step

1. **`pipe(pipefd)`** — Creates two file descriptors:
   - `pipefd[0]` → **read end**
   - `pipefd[1]` → **write end**

2. **`fork()`** — Duplicates the process. Both parent and child inherit both ends.

3. **Close unused ends:**
   - Parent closes `pipefd[0]` (read) — it only writes.
   - Child closes `pipefd[1]` (write) — it only reads.

4. **Parent writes** 5 messages via `write(pipefd[1], ...)` with 1-second pauses.

5. **Child reads** each message via `read(pipefd[0], ...)` — blocks between messages.

6. **Parent closes** `pipefd[1]` after writing — this sends **EOF** to the child.

7. **Parent waits** for the child to exit via `waitpid()` to avoid zombie processes.

### Why close unused ends?

If the parent keeps `pipefd[0]` open and the child keeps `pipefd[1]` open:
- The child's `read()` would **never see EOF** even after the parent finishes writing.
- The OS won't signal EOF until **all** write-end file descriptors are closed.

---

## Code Walkthrough

```c
int pipefd[2];
pipe(pipefd);           // Create pipe
pid_t pid = fork();     // Fork the process

if (pid == 0) {         // Child
    close(pipefd[1]);   // Close write end (child only reads)
    read(pipefd[0], buffer, MSG_SIZE);
    // ... print buffer
} else {                // Parent
    close(pipefd[0]);   // Close read end (parent only writes)
    write(pipefd[1], "Message 1", 9);
    close(pipefd[1]);   // Send EOF
    waitpid(pid, NULL, 0);
}
```

---

## Related System Calls

| Call | Header | Purpose |
|------|--------|---------|
| `pipe(fd[2])` | `<unistd.h>` | Creates anonymous pipe |
| `fork()` | `<unistd.h>` | Duplicates process |
| `read()` | `<unistd.h>` | Reads from pipe |
| `write()` | `<unistd.h>` | Writes to pipe |
| `waitpid()` | `<sys/wait.h>` | Waits for child to exit |
| `close()` | `<unistd.h>` | Closes a file descriptor |
