# Sockets — IPC Demo

> **Assigned to:** Atul (Core Process member)

---

## What are Sockets?

A **socket** is a communication endpoint that allows two processes to exchange data **bidirectionally**. Unlike pipes and shared memory, sockets can communicate over a **network** or locally on the same machine.

Key characteristics:
- **Bidirectional** — data can flow in both directions on the same connection.
- **Flexible** — works between unrelated processes, across machines, or locally.
- **Stream-based** (`SOCK_STREAM`) — reliable, ordered, connection-based (like TCP).
- **Datagram-based** (`SOCK_DGRAM`) — connectionless, unordered (like UDP).
- **Named** — Unix domain sockets use a filesystem path as the address.

---

## Unix Domain vs Network Sockets

| Feature | Unix Domain Socket (`AF_UNIX`) | Network Socket (`AF_INET`) |
|---------|-------------------------------|---------------------------|
| Address | Filesystem path (`/tmp/pbl_socket`) | IP address + port (`127.0.0.1:8080`) |
| Scope | Same machine only | Same machine or over network |
| Performance | Faster (no network stack) | Slightly slower |
| Setup | No IP/port needed | Requires IP/port binding |
| Use case | Local IPC between processes | Distributed systems, microservices |
| Security | File permissions | Firewall / network rules |

This demo uses **Unix Domain Sockets** (`AF_UNIX`, `SOCK_STREAM`) for local IPC — same API as network sockets, just with a file path as the address.

---

## Architecture Diagram

```
  ┌──────────────────────────────────────────────────────────────┐
  │                       File System                            │
  │                  /tmp/pbl_socket  (socket file)              │
  └───────────────────────────┬──────────────────────────────────┘
                              │
          ┌───────────────────┴──────────────────┐
          │                                      │
     server.c                                client.c
          │                                      │
          │  bind() → listen() → accept()        │
          │                      │               │  connect()
          │                      └───────────────┘
          │                                      │
          │    ◄───────────────────────────────  │  send("Hello — message 1")
          │    recv("Hello — message 1")          │
          │    send("[ECHO]: Hello — message 1") ─►
          │                                      │  recv("[ECHO]: Hello — msg 1")
          │              ...                     │         ...
          │                                      │
          │  close() → unlink(socket file)       │  close()
```

---

## Files

| File | Description |
|------|-------------|
| `server.c` | Creates socket, accepts client, echoes 5 messages |
| `client.c` | Connects to server, sends 5 messages, prints echoes |

---

## How to Compile

```bash
# Compile server
gcc server.c -o server

# Compile client
gcc client.c -o client
```

No extra flags needed — socket API is part of the standard C library on Linux.

---

## How to Run

**Start the server first** (it blocks on `accept()`):

```bash
# Terminal 1
./server
```

Then start the client in a second terminal:

```bash
# Terminal 2
./client
```

### Expected output

**Terminal 1 (server):**
```
[Server] Listening on /tmp/pbl_socket
[Server] Waiting for client...

[Server] Client connected.

[Server] Received: "Hello from client — message 1"
[Server] Echoed:   "[ECHO]: Hello from client — message 1"

[Server] Received: "Hello from client — message 2"
[Server] Echoed:   "[ECHO]: Hello from client — message 2"
...
[Server] Session complete. Socket removed.
```

**Terminal 2 (client):**
```
[Client] Connected to server at /tmp/pbl_socket

[Client] Sent:     "Hello from client — message 1"
[Client] Received: "[ECHO]: Hello from client — message 1"

[Client] Sent:     "Hello from client — message 2"
[Client] Received: "[ECHO]: Hello from client — message 2"
...
[Client] Done. Connection closed.
```

---

## Step-by-Step Flow

### Server setup

```c
int fd = socket(AF_UNIX, SOCK_STREAM, 0);  // 1. Create socket
unlink(SOCKET_PATH);                        // 2. Remove stale socket
bind(fd, &addr, sizeof(addr));             // 3. Bind to path
listen(fd, 1);                             // 4. Mark as passive
int client = accept(fd, NULL, NULL);       // 5. Block until client connects
```

### Client setup

```c
int fd = socket(AF_UNIX, SOCK_STREAM, 0);  // 1. Create socket
connect(fd, &addr, sizeof(addr));          // 2. Connect to server
```

### Echo loop

```
Client                            Server
  │                                 │
  │  send("Hello — message N")      │
  │────────────────────────────────►│
  │                                 │  recv → buffer
  │                                 │  prepend "[ECHO]: "
  │◄────────────────────────────────│  send("[ECHO]: Hello — message N")
  │  recv("[ECHO]: ...")            │
  │                                 │
```

---

## Key API Reference

| Function | Purpose |
|----------|---------|
| `socket(domain, type, 0)` | Create a socket |
| `bind(fd, &addr, len)` | Attach socket to an address |
| `listen(fd, backlog)` | Mark socket as server (passive) |
| `accept(fd, NULL, NULL)` | Block and accept incoming connection |
| `connect(fd, &addr, len)` | Connect to a server socket |
| `send(fd, buf, len, flags)` | Send data over socket |
| `recv(fd, buf, size, flags)` | Receive data from socket |
| `close(fd)` | Close socket |
| `unlink(path)` | Remove socket file from filesystem |
