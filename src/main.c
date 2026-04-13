/*  Heuristic Optimization assignment, 2015.
    Adapted by Jérémie Dubois-Lacoste from the ILSLOP implementation
    of Tommaso Schiavinotto:
    ---
    ILSLOP Iterated Local Search Algorithm for Linear Ordering Problem
    Copyright (C) 2004  Tommaso Schiavinotto (tommaso.schiavinotto@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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


void runAllIterImprovementAlgo(long int *s, long long int bestKnown, const char *instanceName) {
    PivotingRule pivotingRules[] = {FIRST_IMPROVEMENT, BEST_IMPROVEMENT};
    Neighborhood neighborhoods[] = {TRANSPOSE, EXCHANGE, INSERT};
    InitialSolution initialSolutions[] = {RANDOM, CHENERY_WATANABE};
    
    const char *pivotingNames[] = {"first", "best"};
    const char *neighborhoodNames[] = {"transpose", "exchange", "insert"};
    const char *initialNames[] = {"random", "cw"};
    
    FILE *csv = fopen("iterative_improvement_results.csv", "a"); // todo csv name as parameter of function 
    if (!csv) {
        fprintf(stderr, "Error opening iterative_improvement_results.csv\n");
        return;
    }
    
    for (int p = 0; p < 2; p++) {
        for (int n = 0; n < 3; n++) {
            for (int i = 0; i < 2; i++) {
                /* Reset solution */
                for (int j = 0; j < PSize; j++) {
                    s[j] = j;
                }
                
                /* Start timer */
                start_timers();
                
                /* Run algorithm */
                iterativeImprovement(s, neighborhoods[n], pivotingRules[p], initialSolutions[i]);
                
                /* Get computation time */
                double computationTime = elapsed_time(VIRTUAL);
                
                /* Compute final cost */
                long long int finalCost = computeCost(s);
                
                /* Compute relative percentage deviation */
                double delta = 100.0 * (bestKnown - finalCost) / bestKnown;
                
                
                /* Write to CSV */
                fprintf(csv, "%s,%s,%s,%s,%lld,%f,%f\n", 
                    instanceName,
                    pivotingNames[p], 
                    neighborhoodNames[n], 
                    initialNames[i],
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

          /* Start timer */
          start_timers();
          
          /* Run VND algorithm */
          VND(s, orders[i]);
          
          /* Get computation time */
          double computationTime = elapsed_time(VIRTUAL);
          
          /* Compute final cost */
          long long int finalCost = computeCost(s);
          
          /* Compute relative percentage deviation */
          double delta = 100.0 * (bestKnown - finalCost) / bestKnown;
          
          /* Write to CSV */
          fprintf(csv, "%s,%s,%lld,%f,%f\n", 
              instanceName,
              orderNames[i],
              finalCost,
              delta,
              computationTime);
          
          fflush(csv);
      }
    
    fclose(csv);
}




int main (int argc, char **argv) 
{
  long int *currentSolution;
  DIR *dir;
  struct dirent *entry;
  char filePath[512];

  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  
  readOpts(argc, argv);

  /* Create CSV headers if files don't exist */
  FILE *csv = fopen("iterative_improvement_results.csv", "r");
  if (!csv) {
      csv = fopen("iterative_improvement_results.csv", "w");
      fprintf(csv, "instance,pivoting_rule,neighborhood,initial_solution,cost,delta_percent,time_seconds\n");
      fclose(csv);
  } else {
      fclose(csv);
  }
  
  csv = fopen("vnd_results.csv", "r");
  if (!csv) {
      csv = fopen("vnd_results.csv", "w");
      fprintf(csv, "instance,order,cost,delta_percent,time_seconds\n");
      fclose(csv);
  } else {
      fclose(csv);
  }

  /* Open instances directory */
  dir = opendir("instances");
  if (!dir) {
      perror("opendir");
      printf("Could not open instances directory\n");
      return 1;
  }

  /* Process each file in the directory */
  while ((entry = readdir(dir)) != NULL) {
      /* Skip directories and hidden files */
      if (entry->d_type == DT_DIR || entry->d_name[0] == '.') 
          continue;

      /* Build full file path */
      snprintf(filePath, sizeof(filePath), "instances/%s", entry->d_name);
      
      printf("\n=== Processing instance: %s ===\n", entry->d_name);
      
      /* Read instance */
      CostMat = readInstance(filePath);

      /* Allocate solution array */
      currentSolution = (long int *)malloc(PSize * sizeof(long int));
      
      /* Read best known value */
      long long int bestKnown = readBestKnown(entry->d_name);
      if (bestKnown < 0) {
          fprintf(stderr, "Warning: Best known value not found for %s\n", entry->d_name);
          free(currentSolution);
          continue;
      }
      
      /* Run all algorithms for this instance */
      // runAllIterImprovementAlgo(currentSolution, bestKnown, entry->d_name);
      
      /* Run VND algorithms for this instance */
      runAllVNDAlgo(currentSolution, bestKnown, entry->d_name);
      
      /* Free solution and cost matrix */
      free(currentSolution);
  }

  closedir(dir);
  printf("\nResults saved to iterative_improvement_results.csv and vnd_results.csv\n");
  return 0;
}
