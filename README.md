# OS Concepts — PBL Project

A college Project-Based Learning (PBL) repository exploring core **Operating System concepts** through hands-on C programs and interactive web-based simulators. Each module covers a distinct OS topic, implemented week by week.

---

## Project Structure

```
ipc-project/
├── pipes/          ← IPC Demo: Anonymous Pipes
├── msgqueue/       ← IPC Demo: POSIX Message Queues
├── sharedmem/      ← IPC Demo: POSIX Shared Memory
├── sockets/        ← IPC Demo: Unix Domain Sockets
├── week2/          ← Multi-Process Simulator (POSIX IPC)
├── week3/          ← CPU Process Scheduling Simulator
├── week4/          ← Deadlock Simulation System (Web GUI)
└── pagereplace/    ← Page Replacement Algorithms (Web GUI)
```

---

## Modules

| Module | Topic | Tech | Status |
|---|---|---|---|
| IPC Demos | Pipes, MsgQueue, Shared Memory, Sockets | C (POSIX) | ✅ Complete |
| Week 2 | Multi-Process Simulator | C (POSIX IPC) | ✅ Complete |
| Week 3 | CPU Scheduling Simulator | C | ✅ Complete |
| Week 4 | Deadlock Simulation System | HTML/CSS/JS | ✅ Complete |
| Page Replace | Page Replacement Algorithms | HTML/CSS/JS | ✅ Complete |

---

## IPC Demos

Four standalone C programs demonstrating different Inter-Process Communication techniques. Each has its own folder, source files, and detailed README.

| Folder | Technique | Key API | Author |
|--------|-----------|---------|--------|
| `pipes/` | Anonymous Pipes | `pipe()`, `fork()` | Anam |
| `msgqueue/` | POSIX Message Queues | `mq_open()`, `mq_send()`, `mq_receive()` | Keith |
| `sharedmem/` | POSIX Shared Memory + Semaphores | `shm_open()`, `mmap()`, `sem_post()` | Brayan |
| `sockets/` | Unix Domain Sockets | `socket()`, `bind()`, `accept()` | Atul |

---

## Team

| Name | GitHub | Role |
|---|---|---|
| Anam | [anamosa101-coder](https://github.com/anamosa101-coder) | Team Lead — Integration & Benchmarking |
| Brayan | [dcunhabrayan007](https://github.com/dcunhabrayan007) | UI Process / Shared Memory |
| Keith | [keith461](https://github.com/keith461) | Logging Process / Message Queues |
| Atul | — | Core Process / Sockets |

---

## How to Run

### C Programs (IPC Demos, Week 2 & 3)

Requires **Linux or macOS** with `gcc` installed. See each folder's `README.md` for specific compile and run instructions.

Quick reference:

```bash
# Pipes
cd pipes && gcc pipe_demo.c -o pipe_demo && ./pipe_demo

# Message Queues (two terminals)
cd msgqueue
gcc sender.c -o sender -lrt
gcc receiver.c -o receiver -lrt
./receiver &   # Terminal 1
./sender       # Terminal 2

# Shared Memory (two terminals)
cd sharedmem
gcc writer.c -o writer -lrt -lpthread
gcc reader.c -o reader -lrt -lpthread
./reader &     # Terminal 1
./writer       # Terminal 2

# Sockets (two terminals)
cd sockets
gcc server.c -o server
gcc client.c -o client
./server &     # Terminal 1
./client       # Terminal 2
```

### Web GUIs (Week 4 & Page Replacement)

No server needed — just open `index.html` in any modern browser:

```
week4/index.html       ← Deadlock Simulation System
pagereplace/index.html ← Page Replacement Algorithms
```

---

## Learning Outcomes

- **IPC mechanisms**: pipes, message queues, shared memory, sockets
- **Process management**: `fork()`, `exec()`, `waitpid()`
- **Synchronization**: semaphores, mutexes
- **CPU scheduling**: FCFS, SJF, Round Robin, Priority
- **Deadlock**: detection, prevention, Banker's Algorithm
- **Memory management**: FIFO, LRU, Optimal page replacement
