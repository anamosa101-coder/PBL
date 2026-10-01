 # IPC Project - PBL

## What this is
This is our PBL (project based learning) project for college. We got assigned 
the topic Inter-Process Communication (IPC) and had to build it out as a team, 
with everyone committing their part weekly on github.

## So wts IPC actually
Basically processes on a computer run separately and don't share memory with 
each other by default. IPC is just the different ways processes can send data 
to each other / talk to each other even though they're separate. Used a lot 
in real systems, like when programs need to work together or share info.

## The techniques we're covering
Split the topic into parts, one person doing each:

| Technique       | What it does                                         | Who's doing it |
|-----------------|-------------------------------------------------------|-----------------|
| Pipes           | one way channel between related processes            | ANAM         |
| Message Queues  | messages sent into a queue, other process reads them  | KEITH       |
| Shared Memory   | processes share a common memory block directly        | BRAYAN         |
| Sockets         | processes talk over network style connections         | ATUL         |

## Folder structure
 project/
├── pipes/
├── msgqueue/
├── sharedmem/
└── sockets/
 each folder has:
- readme explaining that technique
- code for two processes talking to each other using that method (one sends, 
  one receives)

## Team
| Name   | Role      | Working on     |
|--------|-----------|----------------|
| ANAM | Team Lead |    | PIPES
| KEITH | Member    | MESSAGE QUEUES   |
| BRAYAN | Member    | SHARED MEMORY    |
| ATUL | Member    | SOCKETS   |

## How to run stuff
check inside each folder, every technique has its own instructions for 
compiling/running the code
