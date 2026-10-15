/*
 * algorithms.c - Scheduling Algorithm Implementations (Week 3)
 *
 * Implements:
 *   1. FCFS (First-Come-First-Served) - Non-preemptive
 *   2. SJF (Shortest Job First) - Non-preemptive
 *   3. SRTF (Shortest Remaining Time First) - Preemptive SJF
 *   4. Priority Scheduling - Non-preemptive
 *   5. Priority Scheduling - Preemptive
 *   6. Round Robin (RR) with configurable Time Quantum
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

void copy_processes(const Process *src, Process *dest, int n)
{
    for (int i = 0; i < n; i++)
    {
        dest[i] = src[i];
        dest[i].remaining_time = src[i].burst_time;
        dest[i].start_time = -1;
        dest[i].completion_time = 0;
        dest[i].turnaround_time = 0;
        dest[i].waiting_time = 0;
        dest[i].response_time = 0;
        dest[i].is_completed = 0;
    }
}

void append_gantt_segment(ScheduleResult *res, int pid, int start, int end)
{
    if (start >= end)
        return;

    /* Merge adjacent slices for the same process */
    if (res->gantt_count > 0 &&
        res->gantt[res->gantt_count - 1].pid == pid &&
        res->gantt[res->gantt_count - 1].end_time == start)
    {
        res->gantt[res->gantt_count - 1].end_time = end;
        return;
    }

    if (res->gantt_count < MAX_GANTT_SEGMENTS)
    {
        res->gantt[res->gantt_count].pid = pid;
        res->gantt[res->gantt_count].start_time = start;
        res->gantt[res->gantt_count].end_time = end;
        res->gantt_count++;
    }
}

void calculate_metrics(ScheduleResult *res, int n)
{
    res->process_count = n;
    double total_wt = 0.0;
    double total_tat = 0.0;
    double total_rt = 0.0;

    int min_arrival = res->processes[0].arrival_time;
    int max_completion = 0;
    int total_burst = 0;

    for (int i = 0; i < n; i++)
    {
        Process *p = &res->processes[i];
        p->turnaround_time = p->completion_time - p->arrival_time;
        p->waiting_time = p->turnaround_time - p->burst_time;
        p->response_time = p->start_time - p->arrival_time;

        total_wt += p->waiting_time;
        total_tat += p->turnaround_time;
        total_rt += p->response_time;

        total_burst += p->burst_time;

        if (p->arrival_time < min_arrival)
            min_arrival = p->arrival_time;
        if (p->completion_time > max_completion)
            max_completion = p->completion_time;
    }

    res->avg_waiting_time = total_wt / (double)n;
    res->avg_turnaround_time = total_tat / (double)n;
    res->avg_response_time = total_rt / (double)n;

    res->total_simulation_time = max_completion;
    res->total_busy_time = total_burst;
    res->total_idle_time = max_completion - total_burst;

    if (res->total_simulation_time > 0)
    {
        res->cpu_utilization = ((double)res->total_busy_time / (double)res->total_simulation_time) * 100.0;
    }
    else
    {
        res->cpu_utilization = 100.0;
    }
}

/* 1. First-Come-First-Served (FCFS) */
void run_fcfs(const Process *input, int n, ScheduleResult *res)
{
    memset(res, 0, sizeof(ScheduleResult));
    copy_processes(input, res->processes, n);

    int current_time = 0;
    int completed = 0;

    while (completed < n)
    {
        int chosen = -1;
        int earliest_arrival = 1000000;

        for (int i = 0; i < n; i++)
        {
            if (!res->processes[i].is_completed)
            {
                if (res->processes[i].arrival_time <= current_time)
                {
                    if (res->processes[i].arrival_time < earliest_arrival)
                    {
                        earliest_arrival = res->processes[i].arrival_time;
                        chosen = i;
                    }
                }
            }
        }

        /* CPU is idle: advance to next arrival */
        if (chosen == -1)
        {
            int next_arrival = 1000000;
            for (int i = 0; i < n; i++)
            {
                if (!res->processes[i].is_completed && res->processes[i].arrival_time < next_arrival)
                    next_arrival = res->processes[i].arrival_time;
            }
            append_gantt_segment(res, -1, current_time, next_arrival);
            current_time = next_arrival;
            continue;
        }

        Process *p = &res->processes[chosen];
        p->start_time = current_time;
        int start = current_time;
        current_time += p->burst_time;
        p->completion_time = current_time;
        p->is_completed = 1;

        append_gantt_segment(res, p->pid, start, current_time);
        completed++;
    }

    calculate_metrics(res, n);
}

/* 2. Shortest Job First (SJF) - Non-Preemptive */
void run_sjf_non_preemptive(const Process *input, int n, ScheduleResult *res)
{
    memset(res, 0, sizeof(ScheduleResult));
    copy_processes(input, res->processes, n);

    int current_time = 0;
    int completed = 0;

    while (completed < n)
    {
        int chosen = -1;
        int shortest_burst = 1000000;

        for (int i = 0; i < n; i++)
        {
            if (!res->processes[i].is_completed && res->processes[i].arrival_time <= current_time)
            {
                if (res->processes[i].burst_time < shortest_burst)
                {
                    shortest_burst = res->processes[i].burst_time;
                    chosen = i;
                }
                else if (res->processes[i].burst_time == shortest_burst)
                {
                    if (chosen != -1 && res->processes[i].arrival_time < res->processes[chosen].arrival_time)
                        chosen = i;
                }
            }
        }

        if (chosen == -1)
        {
            int next_arrival = 1000000;
            for (int i = 0; i < n; i++)
            {
                if (!res->processes[i].is_completed && res->processes[i].arrival_time < next_arrival)
                    next_arrival = res->processes[i].arrival_time;
            }
            append_gantt_segment(res, -1, current_time, next_arrival);
            current_time = next_arrival;
            continue;
        }

        Process *p = &res->processes[chosen];
        p->start_time = current_time;
        int start = current_time;
        current_time += p->burst_time;
        p->completion_time = current_time;
        p->is_completed = 1;

        append_gantt_segment(res, p->pid, start, current_time);
        completed++;
    }

    calculate_metrics(res, n);
}

/* 3. Shortest Remaining Time First (SRTF) - Preemptive SJF */
void run_srtf_preemptive(const Process *input, int n, ScheduleResult *res)
{
    memset(res, 0, sizeof(ScheduleResult));
    copy_processes(input, res->processes, n);

    int current_time = 0;
    int completed = 0;

    while (completed < n)
    {
        int chosen = -1;
        int shortest_remaining = 1000000;

        for (int i = 0; i < n; i++)
        {
            if (!res->processes[i].is_completed && res->processes[i].arrival_time <= current_time)
            {
                if (res->processes[i].remaining_time < shortest_remaining)
                {
                    shortest_remaining = res->processes[i].remaining_time;
                    chosen = i;
                }
                else if (res->processes[i].remaining_time == shortest_remaining)
                {
                    if (chosen != -1 && res->processes[i].arrival_time < res->processes[chosen].arrival_time)
                        chosen = i;
                }
            }
        }

        if (chosen == -1)
        {
            append_gantt_segment(res, -1, current_time, current_time + 1);
            current_time++;
            continue;
        }

        Process *p = &res->processes[chosen];
        if (p->start_time == -1)
            p->start_time = current_time;

        p->remaining_time--;
        append_gantt_segment(res, p->pid, current_time, current_time + 1);
        current_time++;

        if (p->remaining_time == 0)
        {
            p->completion_time = current_time;
            p->is_completed = 1;
            completed++;
        }
    }

    calculate_metrics(res, n);
}

/* 4. Priority Scheduling - Non-Preemptive (Lower integer = higher priority) */
void run_priority_non_preemptive(const Process *input, int n, ScheduleResult *res)
{
    memset(res, 0, sizeof(ScheduleResult));
    copy_processes(input, res->processes, n);

    int current_time = 0;
    int completed = 0;

    while (completed < n)
    {
        int chosen = -1;
        int highest_priority = 1000000;

        for (int i = 0; i < n; i++)
        {
            if (!res->processes[i].is_completed && res->processes[i].arrival_time <= current_time)
            {
                if (res->processes[i].priority < highest_priority)
                {
                    highest_priority = res->processes[i].priority;
                    chosen = i;
                }
                else if (res->processes[i].priority == highest_priority)
                {
                    if (chosen != -1 && res->processes[i].arrival_time < res->processes[chosen].arrival_time)
                        chosen = i;
                }
            }
        }

        if (chosen == -1)
        {
            int next_arrival = 1000000;
            for (int i = 0; i < n; i++)
            {
                if (!res->processes[i].is_completed && res->processes[i].arrival_time < next_arrival)
                    next_arrival = res->processes[i].arrival_time;
            }
            append_gantt_segment(res, -1, current_time, next_arrival);
            current_time = next_arrival;
            continue;
        }

        Process *p = &res->processes[chosen];
        p->start_time = current_time;
        int start = current_time;
        current_time += p->burst_time;
        p->completion_time = current_time;
        p->is_completed = 1;

        append_gantt_segment(res, p->pid, start, current_time);
        completed++;
    }

    calculate_metrics(res, n);
}

/* 5. Priority Scheduling - Preemptive (Lower integer = higher priority) */
void run_priority_preemptive(const Process *input, int n, ScheduleResult *res)
{
    memset(res, 0, sizeof(ScheduleResult));
    copy_processes(input, res->processes, n);

    int current_time = 0;
    int completed = 0;

    while (completed < n)
    {
        int chosen = -1;
        int highest_priority = 1000000;

        for (int i = 0; i < n; i++)
        {
            if (!res->processes[i].is_completed && res->processes[i].arrival_time <= current_time)
            {
                if (res->processes[i].priority < highest_priority)
                {
                    highest_priority = res->processes[i].priority;
                    chosen = i;
                }
                else if (res->processes[i].priority == highest_priority)
                {
                    if (chosen != -1 && res->processes[i].arrival_time < res->processes[chosen].arrival_time)
                        chosen = i;
                }
            }
        }

        if (chosen == -1)
        {
            append_gantt_segment(res, -1, current_time, current_time + 1);
            current_time++;
            continue;
        }

        Process *p = &res->processes[chosen];
        if (p->start_time == -1)
            p->start_time = current_time;

        p->remaining_time--;
        append_gantt_segment(res, p->pid, current_time, current_time + 1);
        current_time++;

        if (p->remaining_time == 0)
        {
            p->completion_time = current_time;
            p->is_completed = 1;
            completed++;
        }
    }

    calculate_metrics(res, n);
}

/* 6. Round Robin (RR) with Configurable Quantum */
void run_round_robin(const Process *input, int n, int quantum, ScheduleResult *res)
{
    if (quantum <= 0)
        quantum = DEFAULT_TIME_QUANTUM;

    memset(res, 0, sizeof(ScheduleResult));
    copy_processes(input, res->processes, n);

    int ready_queue[MAX_PROCESSES * 100];
    int head = 0;
    int tail = 0;
    int in_queue[MAX_PROCESSES];
    memset(in_queue, 0, sizeof(in_queue));

    int current_time = 0;
    int completed = 0;

    /* Enqueue processes arriving at t=0 */
    for (int i = 0; i < n; i++)
    {
        if (res->processes[i].arrival_time == 0)
        {
            ready_queue[tail++] = i;
            in_queue[i] = 1;
        }
    }

    while (completed < n)
    {
        if (head == tail)
        {
            /* Queue empty: advance to next arrival */
            int next_arrival = 1000000;
            for (int i = 0; i < n; i++)
            {
                if (!res->processes[i].is_completed && res->processes[i].arrival_time < next_arrival)
                    next_arrival = res->processes[i].arrival_time;
            }

            append_gantt_segment(res, -1, current_time, next_arrival);
            current_time = next_arrival;

            for (int i = 0; i < n; i++)
            {
                if (!res->processes[i].is_completed && res->processes[i].arrival_time <= current_time && !in_queue[i])
                {
                    ready_queue[tail++] = i;
                    in_queue[i] = 1;
                }
            }
            continue;
        }

        int idx = ready_queue[head++];
        in_queue[idx] = 0;
        Process *p = &res->processes[idx];

        if (p->start_time == -1)
            p->start_time = current_time;

        int exec_time = (p->remaining_time > quantum) ? quantum : p->remaining_time;
        int start = current_time;
        current_time += exec_time;
        p->remaining_time -= exec_time;

        append_gantt_segment(res, p->pid, start, current_time);

        /* Enqueue newly arrived processes during this execution window */
        for (int i = 0; i < n; i++)
        {
            if (i != idx && !res->processes[i].is_completed && !in_queue[i] &&
                res->processes[i].arrival_time <= current_time)
            {
                ready_queue[tail++] = i;
                in_queue[i] = 1;
            }
        }

        /* If process still has remaining time, push to back of queue */
        if (p->remaining_time > 0)
        {
            ready_queue[tail++] = idx;
            in_queue[idx] = 1;
        }
        else
        {
            p->completion_time = current_time;
            p->is_completed = 1;
            completed++;
        }
    }

    calculate_metrics(res, n);
}
