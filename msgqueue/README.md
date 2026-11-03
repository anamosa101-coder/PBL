# Message Queues — IPC Demo

> **Assigned to:** Keith (Logging Process member)

---

## What are Message Queues?

A **message queue** is a kernel-managed list of messages that allows processes to exchange **typed, structured data asynchronously** — without needing to be related or even running at the same time.

Key characteristics:
- **Asynchronous** — sender and receiver don't need to be running simultaneously.
- **Typed messages** — each message carries a **priority** (POSIX) for ordered retrieval.
- **Kernel-managed** — the queue persists until explicitly unlinked with `mq_unlink()`.
- **Named** — identified by a path like `/pbl_msgq`, accessible to any process with permission.
- **Buffered** — messages are queued; sender doesn't block unless the queue is full.

```
  Sender Process                 Kernel                   Receiver Process
  ┌─────────────┐               ┌──────────────────────┐   ┌─────────────┐
  │             │  mq_send()    │  /pbl_msgq           │   │             │
  │  sender.c   │──────────────►│  [ msg5 ]            │   │ receiver.c  │
  │             │               │  [ msg4 ]            │──►│             │
  └─────────────┘               │  [ msg3 ]            │   └─────────────┘
                                │  [ msg2 ]  mq_receive│
                                │  [ msg1 ]            │
                                └──────────────────────┘
                                   (FIFO within priority)
```

---

## Files

| File | Description |
|------|-------------|
| `sender.c` | Opens `/pbl_msgq`, sends 5 messages |
| `receiver.c` | Opens `/pbl_msgq`, receives 5 messages, unlinks queue |

---

## How to Compile

```bash
# Compile sender
gcc sender.c -o sender -lrt

# Compile receiver
gcc receiver.c -o receiver -lrt
```

> **`-lrt`** links the POSIX real-time library (`librt`) which provides `mq_open`, `mq_send`, `mq_receive`, etc.

---

## How to Run

**Start the receiver first** (it blocks waiting for messages):

```bash
# Terminal 1
./receiver
```

Then start the sender in a second terminal:

```bash
# Terminal 2
./sender
```

### Expected output

**Terminal 1 (receiver):**
```
[Receiver] Opened message queue: /pbl_msgq
[Receiver] Waiting for 5 messages...

[Receiver] Received (prio=1): "Message 1 from sender (PID 5678)"
[Receiver] Received (prio=1): "Message 2 from sender (PID 5678)"
...
[Receiver] All messages received. Queue '/pbl_msgq' unlinked.
```

**Terminal 2 (sender):**
```
[Sender] Opened message queue: /pbl_msgq
[Sender] Sent (prio=1): "Message 1 from sender (PID 5678)"
...
[Sender] All messages sent. Queue still open for receiver.
```

---

## Architecture — Message Flow

```
  mq_open(O_CREAT | O_WRONLY)          mq_open(O_CREAT | O_RDONLY)
        │                                      │
        ▼                                      ▼
   sender.c                              receiver.c
        │                                      │
        │  mq_send("Message 1", prio=1)        │
        │─────────────────────────────────────►│  mq_receive() → "Message 1"
        │  mq_send("Message 2", prio=1)        │
        │─────────────────────────────────────►│  mq_receive() → "Message 2"
        │       ...                            │       ...
        │  mq_close()                          │  mq_close()
                                               │  mq_unlink("/pbl_msgq")
```

---

## Key API Reference

| Function | Purpose |
|----------|---------|
| `mq_open(name, flags, mode, attr)` | Open or create a named message queue |
| `mq_send(mqd, msg, len, prio)` | Send a message (blocks if queue full) |
| `mq_receive(mqd, buf, size, &prio)` | Receive highest-priority message |
| `mq_close(mqd)` | Close the queue descriptor |
| `mq_unlink(name)` | Remove the queue from the kernel |

### `mq_attr` structure

```c
struct mq_attr {
    long mq_flags;    // 0 = blocking
    long mq_maxmsg;   // max messages in queue
    long mq_msgsize;  // max bytes per message
    long mq_curmsgs;  // current message count (read-only)
};
```

---

## Message Queues vs Pipes

| Feature | Pipe | Message Queue |
|---------|------|---------------|
| Direction | One-way | Both (separate queues) |
| Persistence | Gone when process exits | Survives process exit |
| Naming | Anonymous | Named (`/pbl_msgq`) |
| Message boundaries | Byte stream | Discrete messages |
| Priority | None | Yes (0–31) |
| Related processes? | Required | Not required |
