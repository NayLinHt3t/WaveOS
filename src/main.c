
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

typedef struct {
    unsigned long long total;
    unsigned long long idle;
} CpuStats;

typedef struct {
    unsigned long long total_kb;
    unsigned long long available_kb;
} MemoryStats;

/*
 * Read memory information from /proc/meminfo.
 */
int read_memory(MemoryStats *memory) {
    FILE *file = fopen("/proc/meminfo", "r");

    if (file == NULL) {
        perror("Failed to open /proc/meminfo");
        return -1;
    }

    char key[64];
    char unit[16];
    unsigned long long value;

    int found_total = 0;
    int found_available = 0;

    while (fscanf(file, "%63s %llu %15s",
                  key, &value, unit) == 3) {

        if (strcmp(key, "MemTotal:") == 0) {
            memory->total_kb = value;
            found_total = 1;
        }

        if (strcmp(key, "MemAvailable:") == 0) {
            memory->available_kb = value;
            found_available = 1;
        }

        if (found_total && found_available) {
            break;
        }
    }

    fclose(file);

    if (!found_total || !found_available ||
        memory->available_kb > memory->total_kb) {
        fprintf(stderr, "Invalid memory information\n");
        return -1;
    }

    return 0;
}

/*
 * Read cumulative CPU counters from /proc/stat.
 */
int read_cpu(CpuStats *cpu) {
    FILE *file = fopen("/proc/stat", "r");

    if (file == NULL) {
        perror("Failed to open /proc/stat");
        return -1;
    }

    char line[512];

    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return -1;
    }

    fclose(file);

    unsigned long long user, nice, system, idle;
    unsigned long long iowait, irq, softirq, steal;

    int fields = sscanf(
        line,
        "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
        &user, &nice, &system, &idle,
        &iowait, &irq, &softirq, &steal
    );

    if (fields != 8) {
        fprintf(stderr, "Failed to parse CPU statistics\n");
        return -1;
    }

    cpu->total =
        user + nice + system + idle +
        iowait + irq + softirq + steal;

    cpu->idle = idle + iowait;

    return 0;
}

/*
 * Calculate CPU usage between two samples.
 */
double calculate_cpu_usage(CpuStats previous,
                           CpuStats current) {

    if (current.total <= previous.total ||
        current.idle < previous.idle) {
        return -1.0;
    }

    unsigned long long total_delta =
        current.total - previous.total;

    unsigned long long idle_delta =
        current.idle - previous.idle;

    return 100.0 *
        (double)(total_delta - idle_delta) /
        (double)total_delta;
}

void display_stats(CpuStats previous,
                   CpuStats current,
                   MemoryStats memory) {

    double cpu_usage =
        calculate_cpu_usage(previous, current);

    unsigned long long used_kb =
        memory.total_kb - memory.available_kb;

    printf("\033[2J\033[H");

    printf("================================\n");
    printf("          WAVEOS v0.1\n");
    printf("================================\n\n");

    printf("SYSTEM STATUS: MONITORING\n\n");

    printf("CPU\n");

    if (cpu_usage >= 0.0) {
        printf("  Usage:       %.2f%%\n", cpu_usage);
    } else {
        printf("  Usage:       unavailable\n");
    }

    printf("\nMEMORY\n");

    printf("  Total:       %.2f GB\n",
           memory.total_kb / 1048576.0);

    printf("  Used:        %.2f GB\n",
           used_kb / 1048576.0);

    printf("  Available:   %.2f GB\n",
           memory.available_kb / 1048576.0);

    printf("\nMONITOR\n");
    printf("  Status:      ACTIVE\n");
    printf("  Interval:    1000 ms\n");

    printf("\n================================\n");

    fflush(stdout);
}

int main(void) {
    CpuStats previous_cpu;
    CpuStats current_cpu;
    MemoryStats memory;

    if (read_cpu(&previous_cpu) != 0) {
        return EXIT_FAILURE;
    }

    printf("Starting WaveOS System Observer...\n");

    while (1) {
        sleep(1);

        if (read_cpu(&current_cpu) != 0) {
            continue;
        }

        if (read_memory(&memory) != 0) {
            continue;
        }

        display_stats(previous_cpu, current_cpu, memory);

        previous_cpu = current_cpu;
    }

    return EXIT_SUCCESS;
}
