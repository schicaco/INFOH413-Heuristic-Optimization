/*  Heuristic Optimization assignment using parameters */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "instance.h"
#include "utilities.h"
#include "timer.h"
#include "optimization.h"

char *FileName;
PivotingRule PivotingRuleChoice;
Neighborhood NeighborhoodChoice;
InitialSolution InitialSolutionChoice;

void readOpts(int argc, char **argv) {
  int opt;
  struct option long_options[] = {
      {"first", no_argument, 0, 'f'},
      {"best", no_argument, 0, 'b'},
      {"transpose", no_argument, 0, 't'},
      {"exchange", no_argument, 0, 'e'},
      {"insert", no_argument, 0, 'n'},
      {"random", no_argument, 0, 'r'},
      {"cw", no_argument, 0, 'c'},
      {0, 0, 0, 0}
  };

  FileName = NULL;
  PivotingRuleChoice = FIRST_IMPROVEMENT;  // default
  NeighborhoodChoice = TRANSPOSE;            // default
  InitialSolutionChoice = RANDOM;            // default

  int option_index = 0;
  while ((opt = getopt_long(argc, argv, "i:", long_options, &option_index)) > 0) {
      switch (opt) {
          case 'i': /* Instance file */
              FileName = (char *)malloc(strlen(optarg) + 1);
              strncpy(FileName, optarg, strlen(optarg));
              FileName[strlen(optarg)] = '\0';
              break;
          case 'f': /* First improvement */
              PivotingRuleChoice = FIRST_IMPROVEMENT;
              break;
          case 'b': /* Best improvement */
              PivotingRuleChoice = BEST_IMPROVEMENT;
              break;
          case 't': /* Transpose neighborhood */
              NeighborhoodChoice = TRANSPOSE;
              break;
          case 'e': /* Exchange neighborhood */
              NeighborhoodChoice = EXCHANGE;
              break;
          case 'n': /* Insert neighborhood */
              NeighborhoodChoice = INSERT;
              break;
          case 'r': /* Random initial solution */
              InitialSolutionChoice = RANDOM;
              break;
          case 'c': /* Chenery-Watanabe initial solution */
              InitialSolutionChoice = CHENERY_WATANABE;
              break;
          default:
              fprintf(stderr, "Option not recognized.\n");
      }
  }

  if (!FileName) {
      printf("No instance file provided (use -i <instance_name>). Exiting.\n");
      exit(1);
  }
}






long long int readBestKnown(const char *instanceName) {
    FILE *f = fopen("best_known/best_known.txt", "r");
    if (!f) return -1;
    
    char buffer[256];
    char name[128];
    long long int value;
    
    while (fgets(buffer, sizeof(buffer), f)) {
        if (sscanf(buffer, "%s %lld", name, &value) == 2) {
            if (strstr(name, instanceName) != NULL) {
                fclose(f);
                return value;
            }
        }
    }
    fclose(f);
    return -1;
}



int main (int argc, char **argv) 
{
  int j;
  long int *currentSolution;
  int cost, newCost;

  /* Do not buffer output */
  setbuf(stdout,NULL);
  setbuf(stderr,NULL);
  
  if (argc < 2) {
    printf("No instance file provided (use -i <instance_name>). Exiting.\n");
    exit(1);
  }
  
  /* Read parameters */
  readOpts(argc, argv);

  /* Read instance file */
  CostMat = readInstance(FileName);
  printf("Data have been read from instance file. Size of instance = %ld.\n\n", PSize);

  
  /* starts time measurement */
  start_timers();

  /* A solution is just a vector of int with the same size as the instance */
  currentSolution = (long int *)malloc(PSize * sizeof(long int));

  
  /* COMPUTE using iterative improvement */
  iterativeImprovement(currentSolution, NeighborhoodChoice, PivotingRuleChoice, InitialSolutionChoice);
  
  printf("Solution after iterative improvement:\n");
  for (j=0; j < PSize; j++) 
    printf(" %ld", currentSolution[j]);
  printf("\n");
  

  printf("Time elapsed since we started the timer: %g\n\n", elapsed_time(VIRTUAL));


  return 0;
}

