#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <dirent.h>

#include "instance.h"
#include "utilities.h"
#include "timer.h"
#include "optimization.h"

char *FileName;
int RunAllMode = 0;

PivotingRule PivotingRuleChoice;
Neighborhood NeighborhoodChoice;
InitialSolution InitialSolutionChoice;

/* ------------------------------------------------------------ */
/* Read command-line options                                    */
/* ------------------------------------------------------------ */
void readOpts(int argc, char **argv) {
    int opt;
    int option_index = 0;

    struct option long_options[] = {
        {"first",     no_argument, 0, 'f'},
        {"best",      no_argument, 0, 'b'},
        {"transpose", no_argument, 0, 't'},
        {"exchange",  no_argument, 0, 'e'},
        {"insert",    no_argument, 0, 'n'},
        {"random",    no_argument, 0, 'r'},
        {"cw",        no_argument, 0, 'c'},
        {"all",       no_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    FileName = NULL;
    RunAllMode = 0;

    PivotingRuleChoice = FIRST_IMPROVEMENT;
    NeighborhoodChoice = TRANSPOSE;
    InitialSolutionChoice = RANDOM;

    while ((opt = getopt_long(argc, argv, "i:a", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                FileName = (char *)malloc(strlen(optarg) + 1);
                if (!FileName) {
                    fatal("readOpts: malloc failed for FileName.");
                }
                strcpy(FileName, optarg);
                break;

            case 'a':
                RunAllMode = 1;
                break;

            case 'f':
                PivotingRuleChoice = FIRST_IMPROVEMENT;
                break;

            case 'b':
                PivotingRuleChoice = BEST_IMPROVEMENT;
                break;

            case 't':
                NeighborhoodChoice = TRANSPOSE;
                break;

            case 'e':
                NeighborhoodChoice = EXCHANGE;
                break;

            case 'n':
                NeighborhoodChoice = INSERT;
                break;

            case 'r':
                InitialSolutionChoice = RANDOM;
                break;

            case 'c':
                InitialSolutionChoice = CHENERY_WATANABE;
                break;

            default:
                fprintf(stderr,
                    "Usage:\n"
                    "  ./lop -all\n"
                    "  ./lop -i <instance file> [--first|--best] [--transpose|--exchange|--insert] [--random|--cw]\n");
                exit(1);
        }
    }

    /* If not in all-mode, require an instance file */
    if (!RunAllMode && !FileName) {
        fprintf(stderr, "No instance file provided. Use -i <instance file> or -all.\n");
        exit(1);
    }
}


/* ------------------------------------------------------------ */
/* Run all iterative improvement algorithms                     */
/* ------------------------------------------------------------ */
void runAllIterImprovementAlgo(long int *s, long long int bestKnown, const char *instanceName) {
    PivotingRule pivotingRules[] = {FIRST_IMPROVEMENT, BEST_IMPROVEMENT};
    Neighborhood neighborhoods[] = {TRANSPOSE, EXCHANGE, INSERT};
    InitialSolution initialSolutions[] = {RANDOM, CHENERY_WATANABE};

    const char *pivotingNames[] = {"first", "best"};
    const char *neighborhoodNames[] = {"transpose", "exchange", "insert"};
    const char *initialNames[] = {"random", "cw"};

    FILE *csv = fopen("iterative_improvement_results.csv", "a");
    if (!csv) {
        fprintf(stderr, "Error opening iterative_improvement_results.csv\n");
        return;
    }

    for (int p = 0; p < 2; p++) {
        for (int n = 0; n < 3; n++) {
            for (int i = 0; i < 2; i++) {
                start_timers();

                iterativeImprovement(s, neighborhoods[n], pivotingRules[p], initialSolutions[i]);

                double computationTime = elapsed_time(VIRTUAL);
                long long int finalCost = computeCost(s);
                double delta = 100.0 * (bestKnown - finalCost) / bestKnown;

                fprintf(csv, "%s,%s,%s,%s,%lld,%lld,%f,%f\n",
                        instanceName,
                        pivotingNames[p],
                        neighborhoodNames[n],
                        initialNames[i],
                        bestKnown,
                        finalCost,
                        delta,
                        computationTime);

                fflush(csv);
            }
        }
    }

    fclose(csv);
}

/* ------------------------------------------------------------ */
/* Run all VND algorithms                                       */
/* ------------------------------------------------------------ */
void runAllVNDAlgo(long int *s, long long int bestKnown, const char *instanceName) {
    int orders[] = {1, 2};
    const char *orderNames[] = {"order1", "order2"};

    FILE *csv = fopen("vnd_results.csv", "a");
    if (!csv) {
        fprintf(stderr, "Error opening vnd_results.csv\n");
        return;
    }

    for (int i = 0; i < 2; i++) {
        start_timers();

        VND(s, orders[i]);

        double computationTime = elapsed_time(VIRTUAL);
        long long int finalCost = computeCost(s);
        double delta = 100.0 * (bestKnown - finalCost) / bestKnown;

        fprintf(csv, "%s,%s,%lld,%lld,%f,%f\n",
                instanceName,
                orderNames[i],
                bestKnown, 
                finalCost,
                delta,
                computationTime);

        fflush(csv);
    }

    fclose(csv);
}

/* ------------------------------------------------------------ */
/* Single-instance mode                                         */
/* ------------------------------------------------------------ */
void runSingleInstanceMode(void) {
    int j;
    long int *currentSolution;

    CostMat = readInstance(FileName);
    printf("Data have been read from instance file. Size of instance = %ld.\n\n", PSize);

    currentSolution = (long int *)malloc(PSize * sizeof(long int));
    if (!currentSolution) {
        fatal("runSingleInstanceMode: malloc failed.");
    }

    start_timers();

    iterativeImprovement(currentSolution, NeighborhoodChoice, PivotingRuleChoice, InitialSolutionChoice);

    printf("Solution after iterative improvement:\n");
    for (j = 0; j < PSize; j++) {
        printf(" %ld", currentSolution[j]);
    }
    printf("\n");

    printf("Final cost: %lld\n", computeCost(currentSolution));
    printf("Time elapsed: %g\n\n", elapsed_time(VIRTUAL));

    free(currentSolution);
}

/* ------------------------------------------------------------ */
/* All-instances mode                                           */
/* ------------------------------------------------------------ */
void runAllMode(void) {
    long int *currentSolution;
    DIR *dir;
    struct dirent *entry;
    char filePath[512];

    FILE *csv = fopen("iterative_improvement_results.csv", "r");
    if (!csv) {
        csv = fopen("iterative_improvement_results.csv", "w");
        fprintf(csv, "instance,pivoting_rule,neighborhood,initial_solution,cost,best_known,delta_percent,time_seconds\n");
        fclose(csv);
    } else {
        fclose(csv);
    }

    csv = fopen("vnd_results.csv", "r");
    if (!csv) {
        csv = fopen("vnd_results.csv", "w");
        fprintf(csv, "instance,order,cost,best_known,delta_percent,time_seconds\n");
        fclose(csv);
    } else {
        fclose(csv);
    }

    dir = opendir("instances");
    if (!dir) {
        perror("opendir");
        fatal("Could not open instances directory.");
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR || entry->d_name[0] == '.') {
            continue;
        }

        snprintf(filePath, sizeof(filePath), "instances/%s", entry->d_name);

        printf("\n=== Processing instance: %s ===\n", entry->d_name);

        CostMat = readInstance(filePath);

        currentSolution = (long int *)malloc(PSize * sizeof(long int));
        if (!currentSolution) {
            fatal("runAllMode: malloc failed.");
        }

        long long int bestKnown = readBestKnownValue(entry->d_name);
        if (bestKnown < 0) {
            fprintf(stderr, "Warning: Best known value not found for %s\n", entry->d_name);
            free(currentSolution);
            continue;
        }

        runAllIterImprovementAlgo(currentSolution, bestKnown, entry->d_name);
        // runAllVNDAlgo(currentSolution, bestKnown, entry->d_name);

        free(currentSolution);
    }

    closedir(dir);
    printf("\nResults saved to iterative_improvement_results.csv and vnd_results.csv\n");
}

/* ------------------------------------------------------------ */
/* Main                                                         */
/* ------------------------------------------------------------ */
int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    Seed = 123; // fixed seed for reproducibility

    if (argc < 2) {
        fprintf(stderr,
            "Usage:\n"
            "  ./lop -all\n"
            "  ./lop -i <instance file> [--first|--best] [--transpose|--exchange|--insert] [--random|--cw]\n");
        return 1;
    }

    readOpts(argc, argv);

    if (RunAllMode) {
        runAllMode();
    } else {
        runSingleInstanceMode();
    }

    return 0;
}