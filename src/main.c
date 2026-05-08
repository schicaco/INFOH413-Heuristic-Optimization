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
char *FolderName;
int RunAllMode = 0;
int RunII = 0;
int RunVND = 0;
int RunRII = 0;
int RunVNS = 0;

PivotingRule PivotingRuleChoice;
Neighborhood NeighborhoodChoice;
InitialSolution InitialSolutionChoice;


void readOpts(int argc, char **argv) {
    int opt;
    int option_index = 0;

    struct option long_options[] = {
        {"all",       no_argument, 0, 'a'},
        {"folder",    required_argument, 0, 'f'},
        {"ii",        no_argument, 0, '1'},
        {"vnd",       no_argument, 0, '2'},
        {"rii",       no_argument, 0, '3'},
        {"vns",       no_argument, 0, '4'},
        {"first",     no_argument, 0, '5'},
        {"best",      no_argument, 0, '6'},
        {"transpose", no_argument, 0, '7'},
        {"exchange",  no_argument, 0, '8'},
        {"insert",    no_argument, 0, '9'},
        {"random",    no_argument, 0, 'r'},
        {"cw",        no_argument, 0, 'c'},
        {0, 0, 0, 0}
    };

    FileName = NULL;
    FolderName = NULL;
    RunAllMode = 0;
    RunII = 0;
    RunVND = 0;
    RunRII = 0;
    RunVNS = 0;

    PivotingRuleChoice = FIRST_IMPROVEMENT;
    NeighborhoodChoice = TRANSPOSE;
    InitialSolutionChoice = RANDOM;

    while ((opt = getopt_long(argc, argv, "i:f:a", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                FileName = (char *)malloc(strlen(optarg) + 1);
                if (!FileName) {
                    fatal("readOpts: malloc failed for FileName.");
                }
                strcpy(FileName, optarg);
                break;

            case 'f':
                FolderName = (char *)malloc(strlen(optarg) + 1);
                if (!FolderName) {
                    fatal("readOpts: malloc failed for FolderName.");
                }
                strcpy(FolderName, optarg);
                break;
                
            case 'a':
                RunAllMode = 1;
                RunII = 1;
                RunVND = 1;
                RunRII = 1;
                RunVNS = 1;
                break;

            case '1':
                RunII = 1;
                break;

            case '2':
                RunVND = 1;
                break;

            case '3':
                RunRII = 1;
                break;

            case '4':
                RunVNS = 1;
                break;
                
            case '5':
                PivotingRuleChoice = FIRST_IMPROVEMENT;
                break;

            case '6':
                PivotingRuleChoice = BEST_IMPROVEMENT;
                break;

            case '7':
                NeighborhoodChoice = TRANSPOSE;
                break;

            case '8':
                NeighborhoodChoice = EXCHANGE;
                break;

            case '9':
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
                    "  ./lop --all --folder <instances_folder>\n"
                    "  ./lop --ii|--vnd|--rii|--vns --folder <instances_folder>\n"
                    "  ./lop --ii|--vnd|--rii|--vns <instances_folder>\n"
                    "  ./lop -i <instance file> [--first|--best] [--transpose|--exchange|--insert] [--random|--cw]\n");
                exit(1);
        }
    }

    /* Optional positional folder argument for batch mode, e.g. ./lop --rii instances_150 */
    if ((RunAllMode || RunII || RunVND || RunRII || RunVNS) && !FolderName && optind < argc) {
        FolderName = (char *)malloc(strlen(argv[optind]) + 1);
        if (!FolderName) {
            fatal("readOpts: malloc failed for FolderName.");
        }
        strcpy(FolderName, argv[optind]);
    }

    /* If a batch algorithm is selected, require a folder. */
    if ((RunAllMode || RunII || RunVND || RunRII || RunVNS) && !FolderName) {
        fprintf(stderr, "No instances folder provided. Use --folder <instances_folder> or pass it as the last argument.\n");
        fprintf(stderr, "Example: ./lop --rii instances_150\n");
        exit(1);
    }

    /* If no batch algorithm was selected, require an instance file for single-instance mode. */
    if (!RunAllMode && !RunII && !RunVND && !RunRII && !RunVNS && !FileName) {
        fprintf(stderr, "No instance file provided. Use -i <instance file> or select --ii, --vnd, --rii, --vns, or --all with a folder.\n");
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
    
    
    double wp = 0.3;
    printf("Selected algorithms:");
    if (RunII) printf(" II");
    if (RunVND) printf(" VND");
    if (RunRII) {
        printf(" RII");
        printf("chosen wp for RII: %f\n", wp);
        printf("SLS termination time = 500 * average VND time = %.6f seconds\n", terminationTime);
    }
    if (RunVNS) {
        printf(" VNS");
        printf("SLS termination time = 500 * average VND time = %.6f seconds\n", terminationTime);
    }
    printf("\n");

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

        if (RunII) {
            runAllIterImprovementAlgo(currentSolution, bestKnown, entry->d_name);
        }
        if (RunVND) {
            runAllVNDAlgo(currentSolution, bestKnown, entry->d_name);
        }
        if (RunRII) {
            runAllRII(currentSolution, bestKnown, entry->d_name, wp, terminationTime);
        }
        if (RunVNS) {
            runAllVNSAlgo(currentSolution, bestKnown, entry->d_name, terminationTime);
        }
        free(currentSolution);
    }  


    closedir(dir);
    printf("\nSelected experiment results have been saved to the corresponding CSV files.\n");
}


void runQRTDExperiment(const char *instanceName,const char *filePath,long long int bestKnown, char *outputFile) {
    int run;
    double thresholds[] = {0.5};
    int nThresholds = 1;

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

            fprintf(csv, "%s,%s,%.2f,%d,%f,%f\n",
                    instanceName, 
                    "RII_insert_cw",
                    threshold,
                    run, 
                    hitTime, 
                    cutoffTime);

            fflush(csv);
        }

        for (run = 0; run < 25; run++) {
            long int *s = malloc(PSize * sizeof(long int));
            if (!s) fatal("malloc failed in QRTD");

            Seed = 123 + run;

            start_timers();

            double hitTime = VNS_QRTD(s,2,bestKnown,threshold,cutoffTime);

            fprintf(csv, "%s,%s,%.2f,%d,%f,%f\n",
                    instanceName, 
                    "VNS_order2",
                    threshold,
                    run, 
                    hitTime, 
                    cutoffTime);
            fflush(csv);
        }
    }

    fclose(csv);
}


int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    FILE *csv;

    Seed = 123; // fixed seed for reproducibility

    if (argc < 2) {
        fprintf(stderr,
            "Usage:\n"
            "  ./lop --all --folder <instances_folder>\n"
            "  ./lop --ii|--vnd|--rii|--vns --folder <instances_folder>\n"
            "  ./lop --ii|--vnd|--rii|--vns <instances_folder>\n"
            "  ./lop -i <instance file> [--first|--best] [--transpose|--exchange|--insert] [--random|--cw]\n");
        return 1;
    }

    readOpts(argc, argv);

    if (RunII){
        csv = fopen("iterative_improvement_results.csv", "r");
        if (!csv) {
            csv = fopen("iterative_improvement_results.csv", "w");
            fprintf(csv, "instance,pivoting_rule,neighborhood,initial_solution,best_known,cost,delta_percent,time_seconds\n");
            fclose(csv);
        } else {
            fclose(csv);
        }
    }
    if (RunVND) {
        csv = fopen("vnd_results.csv", "r");
        if (!csv) {
            csv = fopen("vnd_results.csv", "w");
            fprintf(csv, "instance,order,best_known,cost,delta_percent,time_seconds\n");
            fclose(csv);
        } else {
            fclose(csv);
        }
    }
    
    if (RunRII) {
        csv = fopen("rii_results.csv", "r");
        if (!csv) {
            csv = fopen("rii_results.csv", "w");
            fprintf(csv, "instance,pivoting_rule,neighborhood,initial_solution,best_known,cost,delta_percent,time_seconds\n");
            fclose(csv);
        } else {
            fclose(csv);
        }
    }

    if (RunVNS) {
        csv = fopen("vns_results.csv", "r");
        if (!csv) {
            csv = fopen("vns_results.csv", "w");
            fprintf(csv, "instance,order,best_known,cost,delta_percent,time_seconds\n");
            fclose(csv);
        } else {
            fclose(csv);
        }
    }
    if (RunAllMode || RunII || RunVND || RunRII || RunVNS) {
        runAllMode(FolderName);
    }
    
    else {
        runSingleInstanceMode();
    }
    
    // QRTD experiments can still be launched manually if needed:
    // runQRTDExperiment("N-stabu2_150", "instances_150/N-stabu2_150", 4327538, "qrt_results_stabu2.csv");
    // runQRTDExperiment("N-t65l11xx_150", "instances_150/N-t65l11xx_150", 253396, "qrt_results_t65l11xx.csv");
    // runQRTDExperiment("N-t70l11xx_150", "instances_150/N-t70l11xx_150", 436862, "qrt_results_t70l11xx_150.csv");
    return 0;
} 