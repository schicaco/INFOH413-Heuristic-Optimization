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
int RunRII = 0;

PivotingRule PivotingRuleChoice;
Neighborhood NeighborhoodChoice;
InitialSolution InitialSolutionChoice;


void readOpts(int argc, char **argv) {
    int opt;
    int option_index = 0;

    struct option long_options[] = {
        {"all",       no_argument, 0, 'a'},
        {"rii",       no_argument, 0, '2'},
        {"first",     no_argument, 0, '3'},
        {"best",      no_argument, 0, '4'},
        {"transpose", no_argument, 0, '5'},
        {"exchange",  no_argument, 0, '6'},
        {"insert",    no_argument, 0, '7'},
        {"random",    no_argument, 0, '8'},
        {"cw",        no_argument, 0, '9'},
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

            case '2':
                RunRII = 1;
                break;
                
            case '3':
                PivotingRuleChoice = FIRST_IMPROVEMENT;
                break;

            case '4':
                PivotingRuleChoice = BEST_IMPROVEMENT;
                break;

            case '5':
                NeighborhoodChoice = TRANSPOSE;
                break;

            case '6':
                NeighborhoodChoice = EXCHANGE;
                break;

            case '7':
                NeighborhoodChoice = INSERT;
                break;

            case '8':
                InitialSolutionChoice = RANDOM;
                break;

            case '9':
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


void runSingleInstanceMode(void) {
    /* Run single instance mode using an iterative improvement */
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



void runAllRII(long int *s, long long int bestKnown, const char *instanceName, double wp, double terminationTime) {
    PivotingRule pivotingRule = BEST_IMPROVEMENT;
    Neighborhood neighborhood = INSERT;
    InitialSolution initialSolution = CHENERY_WATANABE;
    
    const char *pivotingName = "best";
    const char *neighborhoodName = "insert";
    const char *initialName = "cw";

    FILE *csv = fopen("rii_results.csv", "a");
    if (!csv) {
        fprintf(stderr, "Error opening rii_results.csv\n");
        return;
    }

    start_timers();

    randomIterativeImprovement(s, neighborhood, pivotingRule, initialSolution, wp, terminationTime);

    double computationTime = elapsed_time(VIRTUAL);
    long long int finalCost = computeCost(s);
    double delta = 100.0 * (bestKnown - finalCost) / bestKnown;

    fprintf(csv, "%s,%s,%s,%s,%lld,%lld,%f,%f\n",
            instanceName,
            pivotingName,
            neighborhoodName,
            initialName,
            bestKnown,
            finalCost,
            delta,
            computationTime);

    fflush(csv);

    
    fclose(csv);
}



void runAllVNSAlgo(long int *s, long long int bestKnown, const char *instanceName, double terminationTime) {
    int order =  2;
    const char *orderName = "order2";

    FILE *csv = fopen("vns_results.csv", "a");
    if (!csv) {
        fprintf(stderr, "Error opening vns_results.csv\n");
        return;
    }

    start_timers();

    VNS(s, order, terminationTime);

    double computationTime = elapsed_time(VIRTUAL);
    long long int finalCost = computeCost(s);
    double delta = 100.0 * (bestKnown - finalCost) / bestKnown;

    fprintf(csv, "%s,%s,%lld,%lld,%f,%f\n",
            instanceName,
            orderName,
            bestKnown,
            finalCost,
            delta,
            computationTime);

    fflush(csv);


    fclose(csv);
}


void runAllMode(char *directory) {
    /* Run all instances of a directory over the algorithms VND and Iterative Improvement */
    long int *currentSolution;
    DIR *dir;
    struct dirent *entry;
    char filePath[512];

    double averageVNDTime = 0.16120801282051284;
    double terminationTime = averageVNDTime * 500;
    
    printf("SLS termination time = 500 * average VND time = %.6f seconds\n", terminationTime);
    
    double wp = 0.3;
    printf("chosen wp for RII: %f\n", wp);

    // FILE *csv = fopen("iterative_improvement_results.csv", "r");
    // if (!csv) {
    //     csv = fopen("iterative_improvement_results.csv", "w");
    //     fprintf(csv, "instance,pivoting_rule,neighborhood,initial_solution,best_known,cost,delta_percent,time_seconds\n");
    //     fclose(csv);
    // } else {
    //     fclose(csv);
    // }

    // csv = fopen("vnd_results.csv", "r");
    // if (!csv) {
    //     csv = fopen("vnd_results.csv", "w");
    //     fprintf(csv, "instance,order,best_known,cost,delta_percent,time_seconds\n");
    //     fclose(csv);
    // } else {
    //     fclose(csv);
    // }

    FILE *csv = fopen("rii_results.csv", "r");
    if (!csv) {
        csv = fopen("rii_results.csv", "w");
        fprintf(csv, "instance,pivoting_rule,neighborhood,initial_solution,best_known,cost,delta_percent,time_seconds\n");
        fclose(csv);
    } else {
        fclose(csv);
    }

    csv = fopen("vns_results.csv", "r");
    if (!csv) {
        csv = fopen("vns_results.csv", "w");
        fprintf(csv, "instance,order,best_known,cost,delta_percent,time_seconds\n");
        fclose(csv);
    } else {
        fclose(csv);
    }

    dir = opendir(directory);
    if (!dir) {
        perror("opendir");
        fatal("Could not open instances directory.");
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR || entry->d_name[0] == '.') {
            continue;
        }

        snprintf(filePath, sizeof(filePath), "%s/%s", directory, entry->d_name);

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

        // runAllIterImprovementAlgo(currentSolution, bestKnown, entry->d_name);
        // runAllVNDAlgo(currentSolution, bestKnown, entry->d_name);
        runAllRII(currentSolution, bestKnown, entry->d_name, wp, terminationTime);
        runAllVNSAlgo(currentSolution, bestKnown, entry->d_name, terminationTime);
        free(currentSolution);
    }  


    closedir(dir);
    printf("\nResults saved to iterative_improvement_results.csv and vnd_results.csv\n");
}


void runQRTDExperiment(const char *instanceName,const char *filePath,long long int bestKnown, char *outputFile) {
    int run;
    double thresholds[] = {0.1, 0.25, 0.5};
    int nThresholds = 3;

    double averageVNDTime = 0.16120801282051284;
    double terminationTime = averageVNDTime * 500;

    double cutoffTime = 10.0 * terminationTime;

    FILE *csv = fopen(outputFile, "r");
    if (!csv) {
        csv = fopen(outputFile, "w");
        fprintf(csv, "instance,algorithm,threshold_percent,run_number,hit_time_seconds,cutoff_time_seconds\n");
        fclose(csv);
    } else {
        fclose(csv);
    }

    csv = fopen(outputFile, "a");
    if (!csv) {
        fprintf(stderr, "Error opening %s\n", outputFile);
        return;
    }


    CostMat = readInstance(filePath);

    for (int t = 0; t < nThresholds; t++) {
        double threshold = thresholds[t];

        for (run = 0; run < 25; run++) {
            long int *s = malloc(PSize * sizeof(long int));
            if (!s) fatal("malloc failed in QRTD");

            Seed = 123 + run;

            start_timers();

            double hitTime = randomIterativeImprovement_QRTD(s,INSERT,CHENERY_WATANABE,0.3,bestKnown,threshold,cutoffTime);

            fprintf(csv, "%s,RII_insert_cw,%.2f,%d,%f,%f\n",
                    instanceName, 
                    threshold,
                    run, 
                    hitTime, 
                    cutoffTime);

            free(s);
        }

        for (run = 0; run < 25; run++) {
            long int *s = malloc(PSize * sizeof(long int));
            if (!s) fatal("malloc failed in QRTD");

            Seed = 123 + run;

            start_timers();

            double hitTime = VNS_QRTD(s,2,bestKnown,threshold,cutoffTime);

            fprintf(csv, "%s,VNS_order2,%.2f,%d,%f,%f\n",
                    instanceName, 
                    threshold,
                    run, 
                    hitTime, 
                    cutoffTime);
            free(s);
        }
    }

    fclose(csv);
}


int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    Seed = 123; // fixed seed for reproducibility

    if (argc < 2) {
        fprintf(stderr,
            "Usage:\n"
            "  ./lop -rii\n"
            "  ./lop -a\n"
            "  ./lop -i <instance file> [--first|--best] [--transpose|--exchange|--insert] [--random|--cw]\n");
        return 1;
    }

    readOpts(argc, argv);

    if (RunAllMode) {
        // runAllMode("instances_150");

        runQRTDExperiment("N-stabu2_150", "instances_150/N-stabu2_150", 4327538, "qrt_results_stabu2.csv");
        runQRTDExperiment("N-t70d11xn_150", "instances_150/N-t70d11xn_150", 15207063, "qrt_results_t70d11xn.csv");
    } 
    // else if (RunRII){
    //     runAllRII(NULL, 0, "instances_150"); // Placeholder, should read instance and best known value
    // } 
    else {
        runSingleInstanceMode();
    }

    return 0;
} 