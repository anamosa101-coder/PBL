/*
 * scheduler.h - Header for CPU Process Scheduling Simulator (Week 3)
 *
 * Defines core process structures, execution segment records,
 * metrics containers, and function prototypes for all 6 scheduling algorithms.
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PROCESSES 100
#define MAX_GANTT_SEGMENTS 2000
#define DEFAULT_TIME_QUANTUM 2

/* Individual process execution state and metrics */
typedef struct {
    int pid;             /* Process identifier */
    int arrival_time;    /* Arrival Time (AT >= 0) */
    int burst_time;      /* Total CPU Burst Time (BT > 0) */
    int remaining_time;  /* Remaining CPU burst time */
    int priority;        /* Priority level (lower integer = higher priority) */
    int start_time;      /* Time when CPU was first allocated (-1 if unstarted) */
    int completion_time; /* Time when process finishes execution (CT) */
    int turnaround_time; /* Turnaround Time (TAT = CT - AT) */
    int waiting_time;    /* Waiting Time (WT = TAT - BT) */
    int response_time;   /* Response Time (RT = start_time - AT) */
    int is_completed;    /* Termination flag (1 = completed, 0 = pending) */
} Process;

/* Continuous execution slice on the CPU for Gantt chart generation */
typedef struct {
    int pid;             /* Process ID (-1 represents CPU IDLE) */
    int start_time;      /* Interval start time */
    int end_time;        /* Interval end time */
} GanttSegment;

/* Aggregated simulation result for a given algorithm run */
typedef struct {
    Process processes[MAX_PROCESSES];
    int process_count;

    GanttSegment gantt[MAX_GANTT_SEGMENTS];
    int gantt_count;

    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;

    int total_simulation_time;
    int total_busy_time;
    int total_idle_time;
    double cpu_utilization;
} ScheduleResult;

/* Algorithm implementations */
void run_fcfs(const Process *input, int n, ScheduleResult *res);
void run_sjf_non_preemptive(const Process *input, int n, ScheduleResult *res);
void run_srtf_preemptive(const Process *input, int n, ScheduleResult *res);
void run_priority_non_preemptive(const Process *input, int n, ScheduleResult *res);
void run_priority_preemptive(const Process *input, int n, ScheduleResult *res);
void run_round_robin(const Process *input, int n, int quantum, ScheduleResult *res);

/* Visualization and table formatters */
void print_gantt_chart(const ScheduleResult *res);
void print_process_table(const ScheduleResult *res, const char *algo_name);
void print_comparison_matrix(const ScheduleResult results[], const char *algo_names[], int count);

/* Helpers */
void copy_processes(const Process *src, Process *dest, int n);
void append_gantt_segment(ScheduleResult *res, int pid, int start, int end);
void calculate_metrics(ScheduleResult *res, int n);

#endif /* SCHEDULER_H */
