/*  Heuristic Optimization assignment, 2015.
    Adapted by Jérémie Dubois-Lacoste from the ILSLOP implementation
    of Tommaso Schiavinotto:
    ---
    ILSLOP Iterated Lcaol Search Algorithm for Linear Ordering Problem
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
#include <values.h>
#include <string.h>

#include "optimization.h" 
#include "instance.h"
#include "utilities.h"

#ifdef __MINGW32__
#include <float.h>
#define MAX_FLOAT FLT_MAX
#else
#define MAX_FLOAT MAXFLOAT
#endif


long int **CostMat;

/* Cost */

long long int computeCost (long int *s ) {
    /* Compute f(\pi)  */
    
    int h,k;
    long long int sum;
    
    /* Diagonal value are not considered */
    for (sum = 0, h = 0; h < PSize; h++ ) 
    for ( k = h + 1; k < PSize; k++ )
    sum += CostMat[s[h]][s[k]];
    return(sum);
}



/* Initial Solution */

void createRandomSolution(long int *s) {
    int j; 
    long int *random;

    random = generate_random_vector(PSize);
    for ( j = 0 ; j < PSize ; j++ ) {
      s[j] = random[j];
    }
    free ( random );
}


void chenery_and_watanabe(long int *s) {
    int *used = calloc(PSize, sizeof(int)); // used array to track which rows have been selected
    int pos, i, j;

    if (used == NULL) return;

    for (pos = 0; pos < PSize; pos++) { // pos = row position in the solution
        int bestRow = -1;
        long long int bestAttractiveness = LLONG_MIN;

        for (i = 0; i < PSize; i++) {
            if (used[i]) continue;

            long long int attractiveness = 0;
            for (j = 0; j < PSize; j++) {
                if (!used[j] && j != i) {
                    attractiveness += CostMat[i][j];
                }
            }

            if (attractiveness > bestAttractiveness) {
                bestAttractiveness = attractiveness;
                bestRow = i;
            }
        }

        if (bestRow < 0) break;

        s[pos] = bestRow;
        used[bestRow] = 1;
    }

    free(used);
}


/* Neighborhood */

static void transpose(long int *s, int i) {
    long int tmp = s[i];
    s[i] = s[i + 1];
    s[i + 1] = tmp;
}

static void exchange(long int *s, int i, int j) {
    long int tmp = s[i];
    s[i] = s[j];
    s[j] = tmp;
}

static void insert(long int *s, int i, int j) {
    long int tmp;
    int k;

    if (i == j) return;

    tmp = s[i];

    if (i < j) {
        for (k = i; k < j; k++) {
            s[k] = s[k + 1];
        }
        s[j] = tmp;
    } else {
        for (k = i; k > j; k--) {
            s[k] = s[k - 1];
        }
        s[j] = tmp;
    }
}




/* Pivoting rules */

static int firstImprovement(long int *s, Neighborhood neighborhood) {
    int i, j;
    long long int newCost;
    long long int currentCost = computeCost(s);

    switch (neighborhood) {
        case TRANSPOSE:
            for (i = 0; i < PSize - 1; i++) {
                transpose(s, i);
                newCost = computeCost(s);
                if (newCost > currentCost) {
                    return 1;
                }
                transpose(s, i); // undo if no improvement 
            }
            break;

        case EXCHANGE:
            for (i = 0; i < PSize - 1; i++) {
                for (j = i + 1; j < PSize; j++) {
                    exchange(s, i, j);
                    newCost = computeCost(s);
                    if (newCost > currentCost) {
                        return 1;
                    }
                    exchange(s, i, j); // undo if no improvement 
                }
            }
            break;

        case INSERT:
            for (i = 0; i < PSize; i++) {
                for (j = 0; j < PSize; j++) {
                    if (i == j) continue;
                    insert(s, i, j);
                    newCost = computeCost(s);
                    if (newCost > currentCost) {
                        return 1;
                    }
                    insert(s, j, i); // undo if no improvement
                }
            }
            break;
    }   
    return 0;
}



static int bestImprovement(long int *s, Neighborhood neighborhood) {
    int i, j;
    long long int newCost, bestCost = computeCost(s);
    long int *bestSolution = malloc(PSize * sizeof(long int));

    switch (neighborhood) {
        case TRANSPOSE:
            for (i = 0; i < PSize - 1; i++) {
                transpose(s, i);
                newCost = computeCost(s);
                if (newCost > bestCost) {
                    bestCost = newCost;
                    memcpy(bestSolution, s, PSize * sizeof(long int));
                }
                transpose(s, i); // undo
            }
            break;

        case EXCHANGE:
            for (i = 0; i < PSize - 1; i++) {
                for (j = i + 1; j < PSize; j++) {
                    exchange(s, i, j);
                    newCost = computeCost(s);
                    
                    if (newCost > bestCost) {
                        bestCost = newCost;
                        memcpy(bestSolution, s, PSize * sizeof(long int));
                    }
                    exchange(s, i, j); // undo
                }
            }
            break;

        case INSERT:
            for (i = 0; i < PSize; i++) {
                for (j = 0; j < PSize; j++) {
                    if (i == j) continue; 
                    insert(s, i, j);
                    newCost = computeCost(s);

                    if (newCost > bestCost) {
                        bestCost = newCost;
                        memcpy(bestSolution, s, PSize * sizeof(long int));
                    }
                    insert(s, j, i); // undo
                }
            }
            break;
    }
    
    if (bestCost > computeCost(s)) {
        memcpy(s, bestSolution, PSize * sizeof(long int));
        free(bestSolution);
        return 1;
    }
    return 0; 
}


/* Iterative Improvement */

int iterativeImprovement(long int *s, Neighborhood neighborhood, PivotingRule pivotingRule, InitialSolution initialSolution) {
    if (initialSolution == CHENERY_WATANABE) {
        chenery_and_watanabe(s);
    } else {
        createRandomSolution(s);
    }

    if (pivotingRule == FIRST_IMPROVEMENT) {
        firstImprovement(s, neighborhood);
    } else {
        bestImprovement(s, neighborhood);
    }
 
    return 0;
}

/* Variable Neighbourhoof Descent */

void VND(long int *s, int order) {
    /* Using pivoting rule : first improvement 
    Order 1: transpose → exchange → insert 
    Order 2: transpose transpose → insert → exchange
    Initial solution: CW heuristic */
    
    Neighborhood neighborhoods[3] = {TRANSPOSE, EXCHANGE, INSERT}; // default order
    if (order == 1) {
        neighborhoods[0] = TRANSPOSE;
        neighborhoods[1] = EXCHANGE;
        neighborhoods[2] = INSERT;
    } else if (order == 2) {
        neighborhoods[0] = TRANSPOSE;
        neighborhoods[1] = INSERT;
        neighborhoods[2] = EXCHANGE;
    } else {
        fatal("VND: invalid order. Use 1 or 2.");
    } 
    
    /* Start with CW heuristic */
    chenery_and_watanabe(s);
    int i = 0;

    while (i < 3) {
        if (firstImprovement(s, neighborhoods[i])) {
            i = 0; // restart with the first neighborhood
        } else {
            i++; // move to the next neighborhood
        }
    }
}