#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_REFS 10000
#define NUM_INTERVALS 5

static const int INTERVALS[NUM_INTERVALS] = {2000, 4000, 6000, 8000, 10000};

typedef struct {
    int totalFaults;
    double faultRates[NUM_INTERVALS];  // fault rate at each interval
} Stats;

static int read_reference_string(const char *filename, int refs[], int max_refs);
static void simulate_fifo(int frameCount, const int refs[], int nrefs, Stats *stats);
static void simulate_lru(int frameCount, const int refs[], int nrefs, Stats *stats);
static void simulate_opt(int frameCount, const int refs[], int nrefs, Stats *stats);
static int find_in_frames(const int *frames, int frameCount, int page);
static void print_results(FILE *out, int frameCount, const Stats *fifoStats, const Stats *lruStats, const Stats *optStats);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s frame_size input.dat output.dat\n", argv[0]);
        return EXIT_FAILURE;
    }

    int frameCount = atoi(argv[1]);
    if (frameCount <= 0) {
        fprintf(stderr, "Error: frame_size must be positive.\n");
        return EXIT_FAILURE;
    }

    const char *inputFile = argv[2];
    const char *outputFile = argv[3];

    int refs[MAX_REFS];
    int nrefs = read_reference_string(inputFile, refs, MAX_REFS);
    if (nrefs <= 0) {
        fprintf(stderr, "Error: could not read reference string from %s\n", inputFile);
        return EXIT_FAILURE;
    }

    Stats fifoStats, lruStats, optStats;

    simulate_fifo(frameCount, refs, nrefs, &fifoStats);
    simulate_lru(frameCount, refs, nrefs, &lruStats);
    simulate_opt(frameCount, refs, nrefs, &optStats);

    FILE *out = fopen(outputFile, "w");
    if (!out) {
        perror("fopen output");
        return EXIT_FAILURE;
    }

    print_results(out, frameCount, &fifoStats, &lruStats, &optStats);
    fclose(out);

    return EXIT_SUCCESS;
}

// Read up to max_refs integers (one per line) from the input file.
static int read_reference_string(const char *filename, int refs[], int max_refs) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen input");
        return -1;
    }

    int count = 0;
    while (count < max_refs && fscanf(f, "%d", &refs[count]) == 1) {
        count++;
    }

    fclose(f);
    return count;
}

// Helper: find page in frames; return index or -1 if not found.
static int find_in_frames(const int *frames, int frameCount, int page) {
    for (int i = 0; i < frameCount; i++) {
        if (frames[i] == page) {
            return i;
        }
    }
    return -1;
}

// FIFO simulation
static void simulate_fifo(int frameCount, const int refs[], int nrefs, Stats *stats) {
    int *frames = (int *)malloc(sizeof(int) * frameCount);
    if (!frames) {
        fprintf(stderr, "Memory allocation failed in simulate_fifo\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < frameCount; i++) {
        frames[i] = -1;
    }

    int faults = 0;
    int head = 0;  // index of next frame to replace (queue head)
    int intervalIndex = 0;

    for (int i = 0; i < NUM_INTERVALS; i++) {
        stats->faultRates[i] = 0.0;
    }

    for (int i = 0; i < nrefs; i++) {
        int page = refs[i];
        int idx = find_in_frames(frames, frameCount, page);

        if (idx == -1) {
            // Page fault
            faults++;

            // Check for free frame first
            int placed = 0;
            for (int j = 0; j < frameCount; j++) {
                if (frames[j] == -1) {
                    frames[j] = page;
                    placed = 1;
                    break;
                }
            }

            // If no free frame, replace using FIFO
            if (!placed) {
                frames[head] = page;
                head = (head + 1) % frameCount;
            }
        }

        if (intervalIndex < NUM_INTERVALS && (i + 1) == INTERVALS[intervalIndex]) {
            stats->faultRates[intervalIndex] = (double)faults / (double)(i + 1);
            intervalIndex++;
        }
    }

    stats->totalFaults = faults;
    free(frames);
}

// LRU simulation
static void simulate_lru(int frameCount, const int refs[], int nrefs, Stats *stats) {
    int *frames = (int *)malloc(sizeof(int) * frameCount);
    int *lastUsed = (int *)malloc(sizeof(int) * frameCount);
    if (!frames || !lastUsed) {
        fprintf(stderr, "Memory allocation failed in simulate_lru\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < frameCount; i++) {
        frames[i] = -1;
        lastUsed[i] = -1;
    }

    int faults = 0;
    int intervalIndex = 0;

    for (int i = 0; i < NUM_INTERVALS; i++) {
        stats->faultRates[i] = 0.0;
    }

    for (int time = 0; time < nrefs; time++) {
        int page = refs[time];
        int idx = find_in_frames(frames, frameCount, page);

        if (idx != -1) {
            // Hit: update last used time
            lastUsed[idx] = time;
        } else {
            // Miss
            faults++;

            // Try to find a free frame
            int placed = 0;
            for (int j = 0; j < frameCount; j++) {
                if (frames[j] == -1) {
                    frames[j] = page;
                    lastUsed[j] = time;
                    placed = 1;
                    break;
                }
            }

            if (!placed) {
                // Evict least recently used
                int lruIndex = 0;
                int lruTime = lastUsed[0];
                for (int j = 1; j < frameCount; j++) {
                    if (lastUsed[j] < lruTime) {
                        lruTime = lastUsed[j];
                        lruIndex = j;
                    }
                }
                frames[lruIndex] = page;
                lastUsed[lruIndex] = time;
            }
        }

        if (intervalIndex < NUM_INTERVALS && (time + 1) == INTERVALS[intervalIndex]) {
            stats->faultRates[intervalIndex] = (double)faults / (double)(time + 1);
            intervalIndex++;
        }
    }

    stats->totalFaults = faults;

    free(frames);
    free(lastUsed);
}

// Optimal (OPT) simulation
static void simulate_opt(int frameCount, const int refs[], int nrefs, Stats *stats) {
    int *frames = (int *)malloc(sizeof(int) * frameCount);
    if (!frames) {
        fprintf(stderr, "Memory allocation failed in simulate_opt\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < frameCount; i++) {
        frames[i] = -1;
    }

    int faults = 0;
    int intervalIndex = 0;

    for (int i = 0; i < NUM_INTERVALS; i++) {
        stats->faultRates[i] = 0.0;
    }

    for (int i = 0; i < nrefs; i++) {
        int page = refs[i];
        int idx = find_in_frames(frames, frameCount, page);

        if (idx == -1) {
            // Page fault
            faults++;

            // First, check for a free frame
            int placed = 0;
            for (int j = 0; j < frameCount; j++) {
                if (frames[j] == -1) {
                    frames[j] = page;
                    placed = 1;
                    break;
                }
            }

            if (!placed) {
                // Need to evict a page using OPT
                int victim = -1;
                int farthestUse = -1;

                for (int j = 0; j < frameCount; j++) {
                    int currentPage = frames[j];
                    int nextUse = -1;

                    // Find next use of currentPage
                    for (int k = i + 1; k < nrefs; k++) {
                        if (refs[k] == currentPage) {
                            nextUse = k;
                            break;
                        }
                    }

                    if (nextUse == -1) {
                        // This page is never used again: best victim
                        victim = j;
                        break;
                    } else if (nextUse > farthestUse) {
                        farthestUse = nextUse;
                        victim = j;
                    }
                }

                if (victim == -1) {
                    // Should not happen, but just in case
                    victim = 0;
                }

                frames[victim] = page;
            }
        }

        if (intervalIndex < NUM_INTERVALS && (i + 1) == INTERVALS[intervalIndex]) {
            stats->faultRates[intervalIndex] = (double)faults / (double)(i + 1);
            intervalIndex++;
        }
    }

    stats->totalFaults = faults;
    free(frames);
}

// Print results in the requested format
static void print_results(FILE *out, int frameCount,
                          const Stats *fifoStats,
                          const Stats *lruStats,
                          const Stats *optStats) {

    fprintf(out, "===============================================\n");
    fprintf(out, "Page Replacement Algorithm Simulation (frame size = %d)\n", frameCount);
    fprintf(out, "===============================================\n");
    fprintf(out, "Page fault rates\n");

    // Fixed-width aligned header
    fprintf(out,
        "%-12s %15s %10s %10s %10s %10s %10s\n",
        "Algorithm", "Total Faults",
        "2000", "4000", "6000", "8000", "10000"
    );

    fprintf(out, "--------------------------------------------------------------------------------------\n");

    // Helper macro to print one row cleanly
    #define PRINT_ROW(name, stats) \
        fprintf(out, "%-12s %15d %10.3f %10.3f %10.3f %10.3f %10.3f\n", \
            name, stats->totalFaults, \
            stats->faultRates[0], stats->faultRates[1], stats->faultRates[2], \
            stats->faultRates[3], stats->faultRates[4]);

    PRINT_ROW("FIFO", fifoStats);
    PRINT_ROW("LRU",  lruStats);
    PRINT_ROW("OPT",  optStats);

    #undef PRINT_ROW
}

