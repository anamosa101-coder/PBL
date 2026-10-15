/*
 * scheduler.c - Main Entry and Interactive Harness (Week 3)
 *
 * Provides:
 *   - Interactive CLI menu
 *   - Manual process entry with validation
 *   - Pre-loaded benchmark scenarios (Convoy effect, Preemption, RR Sensitivity)
 *   - Random dataset generator
 *   - Single algorithm simulation with Gantt chart display
 *   - Multi-algorithm side-by-side comparison execution
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "scheduler.h"

static Process current_dataset[MAX_PROCESSES];
static int dataset_size = 0;
static int time_quantum = DEFAULT_TIME_QUANTUM;

static void load_preset_1(void)
{
    /* Benchmark 1: Convoy Effect Demonstration */
    dataset_size = 3;
    current_dataset[0] = (Process){.pid = 1, .arrival_time = 0, .burst_time = 24, .priority = 2};
    current_dataset[1] = (Process){.pid = 2, .arrival_time = 0, .burst_time = 3,  .priority = 1};
    current_dataset[2] = (Process){.pid = 3, .arrival_time = 0, .burst_time = 3,  .priority = 3};
    printf("\nLoaded Benchmark 1: Convoy Effect (1 Long Job + 2 Short Jobs at t=0)\n");
}

static void load_preset_2(void)
{
    /* Benchmark 2: Staggered Arrivals & Preemption */
    dataset_size = 4;
    current_dataset[0] = (Process){.pid = 1, .arrival_time = 0, .burst_time = 8, .priority = 3};
    current_dataset[1] = (Process){.pid = 2, .arrival_time = 1, .burst_time = 4, .priority = 1};
    current_dataset[2] = (Process){.pid = 3, .arrival_time = 2, .burst_time = 9, .priority = 4};
    current_dataset[3] = (Process){.pid = 4, .arrival_time = 3, .burst_time = 5, .priority = 2};
    printf("\nLoaded Benchmark 2: Staggered Arrivals & Preemption (4 Jobs)\n");
}

static void load_preset_3(void)
{
    /* Benchmark 3: Round Robin Quantum Sensitivity */
    dataset_size = 5;
    current_dataset[0] = (Process){.pid = 1, .arrival_time = 0, .burst_time = 10, .priority = 3};
    current_dataset[1] = (Process){.pid = 2, .arrival_time = 1, .burst_time = 5,  .priority = 2};
    current_dataset[2] = (Process){.pid = 3, .arrival_time = 2, .burst_time = 8,  .priority = 1};
    current_dataset[3] = (Process){.pid = 4, .arrival_time = 3, .burst_time = 2,  .priority = 4};
    current_dataset[4] = (Process){.pid = 5, .arrival_time = 4, .burst_time = 6,  .priority = 2};
    printf("\nLoaded Benchmark 3: Round Robin Workload (5 Jobs)\n");
}

static void generate_random_dataset(void)
{
    int n;
    printf("Enter number of processes to generate (3-%d): ", MAX_PROCESSES);
    if (scanf("%d", &n) != 1 || n < 1 || n > MAX_PROCESSES)
    {
        printf("Invalid process count.\n");
        return;
    }

    srand((unsigned int)time(NULL));
    dataset_size = n;

    for (int i = 0; i < n; i++)
    {
        current_dataset[i].pid = i + 1;
        current_dataset[i].arrival_time = rand() % 8;        /* AT between 0 and 7 */
        current_dataset[i].burst_time = 1 + (rand() % 12);   /* BT between 1 and 12 */
        current_dataset[i].priority = 1 + (rand() % 5);      /* Priority between 1 and 5 */
    }

    printf("\nGenerated random dataset with %d processes.\n", n);
}

static void manual_input_dataset(void)
{
    int n;
    printf("\nEnter number of processes (1-%d): ", MAX_PROCESSES);
    if (scanf("%d", &n) != 1 || n < 1 || n > MAX_PROCESSES)
    {
        printf("Invalid count.\n");
        return;
    }

    dataset_size = n;
    for (int i = 0; i < n; i++)
    {
        current_dataset[i].pid = i + 1;
        printf("\nProcess %d:\n", i + 1);

        printf("  Arrival Time (>= 0): ");
        while (scanf("%d", &current_dataset[i].arrival_time) != 1 || current_dataset[i].arrival_time < 0)
        {
            printf("  Invalid. Arrival Time must be >= 0: ");
            while (getchar() != '\n');
        }

        printf("  Burst Time (> 0): ");
        while (scanf("%d", &current_dataset[i].burst_time) != 1 || current_dataset[i].burst_time <= 0)
        {
            printf("  Invalid. Burst Time must be > 0: ");
            while (getchar() != '\n');
        }

        printf("  Priority (1 = highest): ");
        while (scanf("%d", &current_dataset[i].priority) != 1 || current_dataset[i].priority < 0)
        {
            printf("  Invalid. Priority must be >= 0: ");
            while (getchar() != '\n');
        }
    }

    printf("\nDataset configured successfully with %d processes.\n", n);
}

static void print_current_dataset(void)
{
    if (dataset_size == 0)
    {
        printf("\nCurrent Dataset: [Empty. Load a preset or enter processes]\n");
        return;
    }

    printf("\nCurrent Dataset (%d processes, Time Quantum = %d):\n", dataset_size, time_quantum);
    printf("--------------------------------------------------\n");
    printf(" PID   Arrival Time   Burst Time   Priority\n");
    printf("--------------------------------------------------\n");
    for (int i = 0; i < dataset_size; i++)
    {
        printf(" P%-3d   %-14d %-12d %-8d\n",
               current_dataset[i].pid,
               current_dataset[i].arrival_time,
               current_dataset[i].burst_time,
               current_dataset[i].priority);
    }
    printf("--------------------------------------------------\n");
}

static void run_all_and_compare(void)
{
    if (dataset_size == 0)
    {
        printf("\nError: Dataset is empty. Load a benchmark or enter processes first.\n");
        return;
    }

    ScheduleResult results[6];
    const char *names[6] = {
        "FCFS",
        "SJF (Non-preemptive)",
        "SRTF (Preemptive SJF)",
        "Priority (Non-preemptive)",
        "Priority (Preemptive)",
        "Round Robin"
    };

    run_fcfs(current_dataset, dataset_size, &results[0]);
    run_sjf_non_preemptive(current_dataset, dataset_size, &results[1]);
    run_srtf_preemptive(current_dataset, dataset_size, &results[2]);
    run_priority_non_preemptive(current_dataset, dataset_size, &results[3]);
    run_priority_preemptive(current_dataset, dataset_size, &results[4]);
    run_round_robin(current_dataset, dataset_size, time_quantum, &results[5]);

    char rr_label[64];
    snprintf(rr_label, sizeof(rr_label), "Round Robin (TQ=%d)", time_quantum);
    names[5] = rr_label;

    print_comparison_matrix(results, names, 6);
}

static void run_single_algorithm_menu(void)
{
    if (dataset_size == 0)
    {
        printf("\nError: Dataset is empty. Load a benchmark or enter processes first.\n");
        return;
    }

    printf("\nSelect Algorithm to Simulate:\n");
    printf("  1. FCFS (First-Come-First-Served)\n");
    printf("  2. SJF (Shortest Job First - Non-preemptive)\n");
    printf("  3. SRTF (Shortest Remaining Time First - Preemptive SJF)\n");
    printf("  4. Priority (Non-preemptive)\n");
    printf("  5. Priority (Preemptive)\n");
    printf("  6. Round Robin (Time Quantum = %d)\n", time_quantum);
    printf("Choice: ");

    int choice;
    if (scanf("%d", &choice) != 1)
        return;

    ScheduleResult res;
    const char *name = "";

    switch (choice)
    {
    case 1:
        name = "FCFS (First-Come-First-Served)";
        run_fcfs(current_dataset, dataset_size, &res);
        break;
    case 2:
        name = "SJF (Shortest Job First - Non-preemptive)";
        run_sjf_non_preemptive(current_dataset, dataset_size, &res);
        break;
    case 3:
        name = "SRTF (Shortest Remaining Time First - Preemptive SJF)";
        run_srtf_preemptive(current_dataset, dataset_size, &res);
        break;
    case 4:
        name = "Priority Scheduling (Non-preemptive)";
        run_priority_non_preemptive(current_dataset, dataset_size, &res);
        break;
    case 5:
        name = "Priority Scheduling (Preemptive)";
        run_priority_preemptive(current_dataset, dataset_size, &res);
        break;
    case 6:
        name = "Round Robin";
        run_round_robin(current_dataset, dataset_size, time_quantum, &res);
        break;
    default:
        printf("Invalid algorithm choice.\n");
        return;
    }

    print_process_table(&res, name);
    print_gantt_chart(&res);
}

int main(int argc, char *argv[])
{
    /* Check command-line shortcuts */
    if (argc > 1)
    {
        if (strcmp(argv[1], "--benchmark") == 0 && argc > 2)
        {
            int b = atoi(argv[2]);
            if (b == 1) load_preset_1();
            else if (b == 2) load_preset_2();
            else if (b == 3) load_preset_3();
            else { printf("Unknown benchmark.\n"); return 1; }

            print_current_dataset();
            run_all_and_compare();
            return 0;
        }
        else if (strcmp(argv[1], "--compare") == 0)
        {
            load_preset_2();
            print_current_dataset();
            run_all_and_compare();
            return 0;
        }
    }

    /* Default: Load Preset 2 initially */
    load_preset_2();

    while (1)
    {
        printf("\n============================================================\n");
        printf("           CPU PROCESS SCHEDULING SIMULATOR                 \n");
        printf("============================================================\n");
        printf(" 1. View Current Process Dataset\n");
        printf(" 2. Enter Processes Manually\n");
        printf(" 3. Load Benchmark 1: Convoy Effect (Long + Short Jobs)\n");
        printf(" 4. Load Benchmark 2: Staggered Arrivals & Preemption\n");
        printf(" 5. Load Benchmark 3: Round Robin Workload\n");
        printf(" 6. Generate Random Process Dataset\n");
        printf(" 7. Run Specific Scheduling Algorithm (Table + Gantt Chart)\n");
        printf(" 8. Run All Algorithms and View Comparison Matrix\n");
        printf(" 9. Change Round Robin Time Quantum (Current: %d)\n", time_quantum);
        printf(" 0. Exit\n");
        printf("============================================================\n");
        printf("Select option: ");

        int opt;
        if (scanf("%d", &opt) != 1)
            break;

        switch (opt)
        {
        case 1:
            print_current_dataset();
            break;
        case 2:
            manual_input_dataset();
            break;
        case 3:
            load_preset_1();
            break;
        case 4:
            load_preset_2();
            break;
        case 5:
            load_preset_3();
            break;
        case 6:
            generate_random_dataset();
            break;
        case 7:
            run_single_algorithm_menu();
            break;
        case 8:
            run_all_and_compare();
            break;
        case 9:
            printf("Enter new Time Quantum (> 0): ");
            int q;
            if (scanf("%d", &q) == 1 && q > 0)
            {
                time_quantum = q;
                printf("Time Quantum updated to %d.\n", time_quantum);
            }
            else
            {
                printf("Invalid Time Quantum.\n");
            }
            break;
        case 0:
            printf("Exiting CPU Scheduling Simulator.\n");
            return 0;
        default:
            printf("Invalid selection. Try again.\n");
            break;
        }
    }

    return 0;
}
