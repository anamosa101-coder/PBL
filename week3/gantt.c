/*
 * gantt.c - Visualization and Reporting Subsystem (Week 3)
 *
 * Implements:
 *   - Text/ASCII Gantt Chart formatting
 *   - Per-process performance metrics table
 *   - Multi-algorithm side-by-side comparison matrix
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheduler.h"

void print_gantt_chart(const ScheduleResult *res)
{
    if (res->gantt_count == 0)
    {
        printf("Gantt Chart: [No execution segments recorded]\n");
        return;
    }

    printf("\n--- Gantt Chart ---\n");

    int i = 0;
    while (i < res->gantt_count)
    {
        int batch_start = i;
        int line_width = 0;

        /* Group segments into terminal rows (up to ~75 characters wide) */
        while (i < res->gantt_count && line_width < 70)
        {
            line_width += 8;
            i++;
        }
        int batch_end = i;

        /* Top border */
        printf("+");
        for (int j = batch_start; j < batch_end; j++)
            printf("-------+");
        printf("\n");

        /* Process labels */
        printf("|");
        for (int j = batch_start; j < batch_end; j++)
        {
            if (res->gantt[j].pid == -1)
                printf(" IDLE  |");
            else
                printf("  P%-2d  |", res->gantt[j].pid);
        }
        printf("\n");

        /* Bottom border */
        printf("+");
        for (int j = batch_start; j < batch_end; j++)
            printf("-------+");
        printf("\n");

        /* Timeline numbers */
        printf("%-7d", res->gantt[batch_start].start_time);
        for (int j = batch_start; j < batch_end; j++)
        {
            printf("%-8d", res->gantt[j].end_time);
        }
        printf("\n\n");
    }
}

void print_process_table(const ScheduleResult *res, const char *algo_name)
{
    printf("\n========================================================================================\n");
    printf(" Algorithm: %s\n", algo_name);
    printf("========================================================================================\n");
    printf(" PID   Arrival   Burst   Priority   Start   Completion   Turnaround   Waiting   Response\n");
    printf("----------------------------------------------------------------------------------------\n");

    for (int i = 0; i < res->process_count; i++)
    {
        const Process *p = &res->processes[i];
        printf(" P%-3d   %-7d   %-5d   %-8d   %-5d   %-10d   %-10d   %-7d   %-8d\n",
               p->pid, p->arrival_time, p->burst_time, p->priority,
               p->start_time, p->completion_time, p->turnaround_time,
               p->waiting_time, p->response_time);
    }

    printf("----------------------------------------------------------------------------------------\n");
    printf(" Average Waiting Time    : %.2f ms\n", res->avg_waiting_time);
    printf(" Average Turnaround Time : %.2f ms\n", res->avg_turnaround_time);
    printf(" Average Response Time   : %.2f ms\n", res->avg_response_time);
    printf(" Total Simulation Time   : %d ms\n", res->total_simulation_time);
    printf(" CPU Busy Time           : %d ms\n", res->total_busy_time);
    printf(" CPU Idle Time           : %d ms\n", res->total_idle_time);
    printf(" CPU Utilization         : %.2f %%\n", res->cpu_utilization);
    printf("========================================================================================\n");
}

void print_comparison_matrix(const ScheduleResult results[], const char *algo_names[], int count)
{
    printf("\n=============================================================================================\n");
    printf("                        MULTI-ALGORITHM COMPARATIVE EVALUATION\n");
    printf("=============================================================================================\n");
    printf(" %-28s | %-10s | %-11s | %-10s | %-11s | %-9s\n",
           "Algorithm", "Avg WT(ms)", "Avg TAT(ms)", "Avg RT(ms)", "CPU Util %", "Idle Time");
    printf("---------------------------------------------------------------------------------------------\n");

    for (int i = 0; i < count; i++)
    {
        printf(" %-28s | %10.2f | %11.2f | %10.2f | %10.2f%% | %7d ms\n",
               algo_names[i],
               results[i].avg_waiting_time,
               results[i].avg_turnaround_time,
               results[i].avg_response_time,
               results[i].cpu_utilization,
               results[i].total_idle_time);
    }

    printf("=============================================================================================\n");
}
