/*
 * core.c - Core Process of the multi-process simulator
 *
 * Implements CPU execution, Registers, Memory, Stack, and Queue subsystems.
 * Acts as the intermediary between UI and Logger:
 *
 *   UI    --/pbl_manager_core--> Core
 *   UI    <--/pbl_core_manager-- Core
 *   Core  --/pbl_core_logger-->  Logger
 *   Core  <--/pbl_logger_core--  Logger (ACKs)
 *
 * START ORDER:  Logger  ->  Core  ->  UI
 *   1. Logger starts first and creates logger queues.
 *   2. Core starts second, opens logger queues, and creates UI queues.
 *   3. UI starts third and connects to UI queues.
 *
 * Build:  gcc -Wall -Wextra -o core core.c -lrt
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define MSG_SIZE 256
#define QUEUE_MAX_MSG 10
#define TIMEOUT_SEC 2

#define MANAGER_TO_CORE "/pbl_manager_core"
#define CORE_TO_MANAGER "/pbl_core_manager"
#define CORE_TO_LOGGER  "/pbl_core_logger"
#define LOGGER_TO_CORE  "/pbl_logger_core"

/* Subsystem capacities */
#define NUM_REGISTERS 8
#define MEMORY_SIZE   256
#define STACK_CAPACITY 64
#define QUEUE_CAPACITY 64

/* State storage */
static int registers[NUM_REGISTERS];
static int memory[MEMORY_SIZE];

static int stack[STACK_CAPACITY];
static int stack_top = 0; /* Index of next available slot */

static int fifo_queue[QUEUE_CAPACITY];
static int queue_front = 0;
static int queue_rear = 0;
static int queue_count = 0;

static volatile sig_atomic_t running = 1;

static void on_signal(int sig)
{
    (void)sig;
    running = 0;
}

static void deadline_in(struct timespec *ts, int seconds)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec += seconds;
}

/* Helper to parse register name (R0 through R7) */
static int parse_register(const char *token)
{
    if (token == NULL || (token[0] != 'R' && token[0] != 'r'))
        return -1;

    int idx = token[1] - '0';
    if (idx < 0 || idx >= NUM_REGISTERS || token[2] != '\0')
        return -1;

    return idx;
}

/* Dispatch log message to Logger and wait for confirmation */
static int send_log(mqd_t to_logger, mqd_t from_logger, const char *log_text)
{
    struct timespec ts;
    char ack[MSG_SIZE];
    ssize_t rec;

    deadline_in(&ts, TIMEOUT_SEC);

    while (mq_timedsend(to_logger, log_text, strlen(log_text) + 1, 0, &ts) == -1)
    {
        if (errno == EINTR)
            continue;
        perror("Core: mq_timedsend to logger");
        return -1;
    }

    deadline_in(&ts, TIMEOUT_SEC);
    do
    {
        rec = mq_timedreceive(from_logger, ack, sizeof(ack), NULL, &ts);
    } while (rec == -1 && errno == EINTR);

    if (rec == -1)
    {
        if (errno == ETIMEDOUT)
            fprintf(stderr, "Core: Logger ACK timed out.\n");
        else
            perror("Core: mq_timedreceive from logger");
        return -1;
    }

    ack[rec] = '\0';
    return 0;
}

/* Dispatch reply back to UI */
static int send_reply(mqd_t to_ui, const char *reply_text)
{
    struct timespec ts;
    deadline_in(&ts, TIMEOUT_SEC);

    while (mq_timedsend(to_ui, reply_text, strlen(reply_text) + 1, 0, &ts) == -1)
    {
        if (errno == EINTR)
            continue;
        perror("Core: mq_timedsend to UI");
        return -1;
    }

    return 0;
}

/* Process individual command, log it, and format reply */
static void execute_command(const char *cmd_line, char *reply, size_t reply_sz,
                            mqd_t to_logger, mqd_t from_logger, int *terminate_flag)
{
    char copy[MSG_SIZE];
    char log_buf[MSG_SIZE];
    strncpy(copy, cmd_line, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';

    char *op = strtok(copy, " \t\r\n");
    if (op == NULL)
    {
        snprintf(reply, reply_sz, "ERROR: Empty command");
        return;
    }

    /* Convert op to upper case */
    for (char *p = op; *p; p++)
        *p = (char)toupper((unsigned char)*p);

    /* 1. LOAD <Rx> <val> */
    if (strcmp(op, "LOAD") == 0)
    {
        char *r_tok = strtok(NULL, " \t\r\n");
        char *v_tok = strtok(NULL, " \t\r\n");
        int r = parse_register(r_tok);

        if (r == -1 || v_tok == NULL)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: LOAD <R0-R7> <integer>");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid LOAD syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        char *endptr;
        long val = strtol(v_tok, &endptr, 10);
        if (*endptr != '\0')
        {
            snprintf(reply, reply_sz, "ERROR: Invalid value: %s", v_tok);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid integer in LOAD: %s", v_tok);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        registers[r] = (int)val;
        snprintf(reply, reply_sz, "OK: R%d = %d", r, registers[r]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] LOAD R%d = %d", r, registers[r]);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 2. ADD <Rx> <Ry> */
    else if (strcmp(op, "ADD") == 0)
    {
        char *r1_tok = strtok(NULL, " \t\r\n");
        char *r2_tok = strtok(NULL, " \t\r\n");
        int r1 = parse_register(r1_tok);
        int r2 = parse_register(r2_tok);

        if (r1 == -1 || r2 == -1)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: ADD <Rx> <Ry>");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid ADD syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        registers[r1] += registers[r2];
        snprintf(reply, reply_sz, "OK: R%d = %d", r1, registers[r1]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] ADD R%d, R%d -> R%d = %d", r1, r2, r1, registers[r1]);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 3. SUB <Rx> <Ry> */
    else if (strcmp(op, "SUB") == 0)
    {
        char *r1_tok = strtok(NULL, " \t\r\n");
        char *r2_tok = strtok(NULL, " \t\r\n");
        int r1 = parse_register(r1_tok);
        int r2 = parse_register(r2_tok);

        if (r1 == -1 || r2 == -1)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: SUB <Rx> <Ry>");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid SUB syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        registers[r1] -= registers[r2];
        snprintf(reply, reply_sz, "OK: R%d = %d", r1, registers[r1]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] SUB R%d, R%d -> R%d = %d", r1, r2, r1, registers[r1]);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 4. MUL <Rx> <Ry> */
    else if (strcmp(op, "MUL") == 0)
    {
        char *r1_tok = strtok(NULL, " \t\r\n");
        char *r2_tok = strtok(NULL, " \t\r\n");
        int r1 = parse_register(r1_tok);
        int r2 = parse_register(r2_tok);

        if (r1 == -1 || r2 == -1)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: MUL <Rx> <Ry>");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid MUL syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        registers[r1] *= registers[r2];
        snprintf(reply, reply_sz, "OK: R%d = %d", r1, registers[r1]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] MUL R%d, R%d -> R%d = %d", r1, r2, r1, registers[r1]);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 5. DIV <Rx> <Ry> */
    else if (strcmp(op, "DIV") == 0)
    {
        char *r1_tok = strtok(NULL, " \t\r\n");
        char *r2_tok = strtok(NULL, " \t\r\n");
        int r1 = parse_register(r1_tok);
        int r2 = parse_register(r2_tok);

        if (r1 == -1 || r2 == -1)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: DIV <Rx> <Ry>");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid DIV syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        if (registers[r2] == 0)
        {
            snprintf(reply, reply_sz, "ERROR: Division by zero (R%d is 0)", r2);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Division by zero attempted: DIV R%d R%d", r1, r2);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        registers[r1] /= registers[r2];
        snprintf(reply, reply_sz, "OK: R%d = %d", r1, registers[r1]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] DIV R%d, R%d -> R%d = %d", r1, r2, r1, registers[r1]);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 6. STORE <Rx> <addr> */
    else if (strcmp(op, "STORE") == 0)
    {
        char *r_tok = strtok(NULL, " \t\r\n");
        char *a_tok = strtok(NULL, " \t\r\n");
        int r = parse_register(r_tok);

        if (r == -1 || a_tok == NULL)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: STORE <Rx> <addr 0-%d>", MEMORY_SIZE - 1);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid STORE syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int addr = atoi(a_tok);
        if (addr < 0 || addr >= MEMORY_SIZE)
        {
            snprintf(reply, reply_sz, "ERROR: Address out of bounds (0-%d)", MEMORY_SIZE - 1);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] STORE out of bounds: Address %d", addr);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        memory[addr] = registers[r];
        snprintf(reply, reply_sz, "OK: Memory[%d] = %d (from R%d)", addr, memory[addr], r);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] STORE R%d (%d) -> Memory[%d]", r, registers[r], addr);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 7. READ <addr> */
    else if (strcmp(op, "READ") == 0)
    {
        char *a_tok = strtok(NULL, " \t\r\n");
        if (a_tok == NULL)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: READ <addr 0-%d>", MEMORY_SIZE - 1);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid READ syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int addr = atoi(a_tok);
        if (addr < 0 || addr >= MEMORY_SIZE)
        {
            snprintf(reply, reply_sz, "ERROR: Address out of bounds (0-%d)", MEMORY_SIZE - 1);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] READ out of bounds: Address %d", addr);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        snprintf(reply, reply_sz, "OK: Memory[%d] = %d", addr, memory[addr]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] READ Memory[%d] -> %d", addr, memory[addr]);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 8. WRITE <addr> <val> */
    else if (strcmp(op, "WRITE") == 0)
    {
        char *a_tok = strtok(NULL, " \t\r\n");
        char *v_tok = strtok(NULL, " \t\r\n");

        if (a_tok == NULL || v_tok == NULL)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: WRITE <addr 0-%d> <value>", MEMORY_SIZE - 1);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid WRITE syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int addr = atoi(a_tok);
        int val = atoi(v_tok);
        if (addr < 0 || addr >= MEMORY_SIZE)
        {
            snprintf(reply, reply_sz, "ERROR: Address out of bounds (0-%d)", MEMORY_SIZE - 1);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] WRITE out of bounds: Address %d", addr);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        memory[addr] = val;
        snprintf(reply, reply_sz, "OK: Memory[%d] = %d", addr, memory[addr]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] WRITE Memory[%d] = %d", addr, val);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 9. PUSH <val> */
    else if (strcmp(op, "PUSH") == 0)
    {
        char *v_tok = strtok(NULL, " \t\r\n");
        if (v_tok == NULL)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: PUSH <value>");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid PUSH syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        if (stack_top >= STACK_CAPACITY)
        {
            snprintf(reply, reply_sz, "ERROR: Stack Overflow (Capacity: %d)", STACK_CAPACITY);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Stack Overflow on PUSH %s", v_tok);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int val = atoi(v_tok);
        stack[stack_top++] = val;
        snprintf(reply, reply_sz, "OK: Pushed %d (Stack Size: %d)", val, stack_top);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] PUSH %d -> Stack Size: %d", val, stack_top);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 10. POP */
    else if (strcmp(op, "POP") == 0)
    {
        if (stack_top <= 0)
        {
            snprintf(reply, reply_sz, "ERROR: Stack Underflow (Stack is empty)");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Stack Underflow on POP");
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int val = stack[--stack_top];
        snprintf(reply, reply_sz, "OK: Popped %d (Stack Size: %d)", val, stack_top);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] POP -> %d (Stack Size: %d)", val, stack_top);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 11. PEEK */
    else if (strcmp(op, "PEEK") == 0)
    {
        if (stack_top <= 0)
        {
            snprintf(reply, reply_sz, "ERROR: Stack is empty");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] PEEK on empty stack");
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int val = stack[stack_top - 1];
        snprintf(reply, reply_sz, "OK: Stack Top = %d", val);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] PEEK -> %d", val);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 12. DISPLAY_STACK */
    else if (strcmp(op, "DISPLAY_STACK") == 0)
    {
        if (stack_top == 0)
        {
            snprintf(reply, reply_sz, "OK: Stack is empty");
        }
        else
        {
            int offset = snprintf(reply, reply_sz, "OK: Stack [Top -> Bottom]: ");
            for (int i = stack_top - 1; i >= 0 && offset < (int)reply_sz - 10; i--)
            {
                offset += snprintf(reply + offset, reply_sz - offset, "%d%s",
                                   stack[i], (i > 0) ? ", " : "");
            }
        }
        snprintf(log_buf, sizeof(log_buf), "[EXEC] DISPLAY_STACK (Size: %d)", stack_top);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 13. ENQUEUE <val> */
    else if (strcmp(op, "ENQUEUE") == 0)
    {
        char *v_tok = strtok(NULL, " \t\r\n");
        if (v_tok == NULL)
        {
            snprintf(reply, reply_sz, "ERROR: Usage: ENQUEUE <value>");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Invalid ENQUEUE syntax: %s", cmd_line);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        if (queue_count >= QUEUE_CAPACITY)
        {
            snprintf(reply, reply_sz, "ERROR: Queue Overflow (Capacity: %d)", QUEUE_CAPACITY);
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Queue Overflow on ENQUEUE %s", v_tok);
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int val = atoi(v_tok);
        fifo_queue[queue_rear] = val;
        queue_rear = (queue_rear + 1) % QUEUE_CAPACITY;
        queue_count++;

        snprintf(reply, reply_sz, "OK: Enqueued %d (Queue Size: %d)", val, queue_count);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] ENQUEUE %d -> Queue Size: %d", val, queue_count);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 14. DEQUEUE */
    else if (strcmp(op, "DEQUEUE") == 0)
    {
        if (queue_count <= 0)
        {
            snprintf(reply, reply_sz, "ERROR: Queue Underflow (Queue is empty)");
            snprintf(log_buf, sizeof(log_buf), "[ERROR] Queue Underflow on DEQUEUE");
            send_log(to_logger, from_logger, log_buf);
            return;
        }

        int val = fifo_queue[queue_front];
        queue_front = (queue_front + 1) % QUEUE_CAPACITY;
        queue_count--;

        snprintf(reply, reply_sz, "OK: Dequeued %d (Queue Size: %d)", val, queue_count);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] DEQUEUE -> %d (Queue Size: %d)", val, queue_count);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 15. DISPLAY_QUEUE */
    else if (strcmp(op, "DISPLAY_QUEUE") == 0)
    {
        if (queue_count == 0)
        {
            snprintf(reply, reply_sz, "OK: Queue is empty");
        }
        else
        {
            int offset = snprintf(reply, reply_sz, "OK: Queue [Front -> Rear]: ");
            for (int i = 0; i < queue_count && offset < (int)reply_sz - 10; i++)
            {
                int idx = (queue_front + i) % QUEUE_CAPACITY;
                offset += snprintf(reply + offset, reply_sz - offset, "%d%s",
                                   fifo_queue[idx], (i < queue_count - 1) ? ", " : "");
            }
        }
        snprintf(log_buf, sizeof(log_buf), "[EXEC] DISPLAY_QUEUE (Size: %d)", queue_count);
        send_log(to_logger, from_logger, log_buf);
    }
    /* 16. REGISTERS */
    else if (strcmp(op, "REGISTERS") == 0)
    {
        snprintf(reply, reply_sz, "OK: R0=%d R1=%d R2=%d R3=%d R4=%d R5=%d R6=%d R7=%d",
                 registers[0], registers[1], registers[2], registers[3],
                 registers[4], registers[5], registers[6], registers[7]);
        snprintf(log_buf, sizeof(log_buf), "[EXEC] REGISTERS displayed");
        send_log(to_logger, from_logger, log_buf);
    }
    /* 17. EXIT */
    else if (strcmp(op, "EXIT") == 0)
    {
        *terminate_flag = 1;
        snprintf(log_buf, sizeof(log_buf), "EXIT");
        send_log(to_logger, from_logger, log_buf);
        snprintf(reply, reply_sz, "CORE TERMINATED");
    }
    /* Unknown command */
    else
    {
        snprintf(reply, reply_sz, "ERROR: Unknown command '%s'. Type HELP for command list.", op);
        snprintf(log_buf, sizeof(log_buf), "[ERROR] Unknown command received: %s", cmd_line);
        send_log(to_logger, from_logger, log_buf);
    }
}

int main(void)
{
    mqd_t to_logger = (mqd_t)-1;
    mqd_t from_logger = (mqd_t)-1;
    mqd_t from_ui = (mqd_t)-1;
    mqd_t to_ui = (mqd_t)-1;

    struct mq_attr attr;
    struct sigaction sa;
    char cmd_buf[MSG_SIZE];
    char reply_buf[MSG_SIZE];
    int exit_requested = 0;
    int exit_code = EXIT_SUCCESS;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    memset(registers, 0, sizeof(registers));
    memset(memory, 0, sizeof(memory));

    printf("====================================\n");
    printf("           CORE PROCESS             \n");
    printf("====================================\n");
    printf("Connecting to Logger...\n");

    /* 1. Connect to Logger queues (Logger must be started first) */
    to_logger = mq_open(CORE_TO_LOGGER, O_WRONLY);
    if (to_logger == (mqd_t)-1)
    {
        perror("mq_open Core->Logger");
        fprintf(stderr, "Error: Start the Logger process before starting Core.\n");
        return EXIT_FAILURE;
    }

    from_logger = mq_open(LOGGER_TO_CORE, O_RDONLY);
    if (from_logger == (mqd_t)-1)
    {
        perror("mq_open Logger->Core");
        mq_close(to_logger);
        return EXIT_FAILURE;
    }

    /* 2. Setup and create UI queues (Core owns and removes UI queues) */
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg = QUEUE_MAX_MSG;
    attr.mq_msgsize = MSG_SIZE;

    mq_unlink(MANAGER_TO_CORE);
    mq_unlink(CORE_TO_MANAGER);

    from_ui = mq_open(MANAGER_TO_CORE, O_CREAT | O_RDONLY, 0600, &attr);
    if (from_ui == (mqd_t)-1)
    {
        perror("mq_open Manager->Core");
        exit_code = EXIT_FAILURE;
        goto done;
    }

    to_ui = mq_open(CORE_TO_MANAGER, O_CREAT | O_WRONLY, 0600, &attr);
    if (to_ui == (mqd_t)-1)
    {
        perror("mq_open Core->Manager");
        exit_code = EXIT_FAILURE;
        goto done;
    }

    printf("Core initialized successfully.\n");
    printf("Now ready for UI connections.\n");
    fflush(stdout);

    send_log(to_logger, from_logger, "[INFO] Core process started and ready.");

    /* Main request-reply loop */
    while (running && !exit_requested)
    {
        ssize_t rec = mq_receive(from_ui, cmd_buf, sizeof(cmd_buf), NULL);
        if (rec == -1)
        {
            if (errno == EINTR)
                continue;
            perror("Core: mq_receive from UI");
            break;
        }

        cmd_buf[rec] = '\0';

        execute_command(cmd_buf, reply_buf, sizeof(reply_buf),
                        to_logger, from_logger, &exit_requested);

        if (send_reply(to_ui, reply_buf) == -1)
        {
            fprintf(stderr, "Core: Failed to send reply to UI.\n");
            break;
        }
    }

    printf("Core process shutting down...\n");
    fflush(stdout);

done:
    if (to_ui != (mqd_t)-1)
        mq_close(to_ui);
    if (from_ui != (mqd_t)-1)
        mq_close(from_ui);
    if (to_logger != (mqd_t)-1)
        mq_close(to_logger);
    if (from_logger != (mqd_t)-1)
        mq_close(from_logger);

    /* Unlink UI queues */
    mq_unlink(MANAGER_TO_CORE);
    mq_unlink(CORE_TO_MANAGER);

    return exit_code;
}
