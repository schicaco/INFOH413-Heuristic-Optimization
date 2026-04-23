#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "optimization.h"
#include "instance.h"
#include "utilities.h"

long int **CostMat;


/* COST*/

long long int computeCost(long int *s) {
    int h, k;
    long long int sum = 0;

    for (h = 0; h < PSize; h++) {
        for (k = h + 1; k < PSize; k++) {
            sum += CostMat[s[h]][s[k]];
        }
    }
    return sum;
}


/* INITIAL SOLUTIONS */

void createRandomSolution(long int *s) {
    int j;
    long int *random = generate_random_vector(PSize);

    for (j = 0; j < PSize; j++) {
        s[j] = random[j];
    }
    free(random);
}


void chenery_and_watanabe(long int *s) {
    int pos, i, j;
    int *used = (int *)calloc(PSize, sizeof(int));

    if (used == NULL) {
        fatal("chenery_and_watanabe: memory allocation failed.");
    }

    for (pos = 0; pos < PSize; pos++) {
        int bestRow = -1;
        long long int bestScore = LLONG_MIN;

        for (i = 0; i < PSize; i++) {
            long long int score = 0;

            if (used[i]) continue;

            for (j = 0; j < PSize; j++) {
                if (!used[j] && j != i) {
                    score += CostMat[i][j];
                }
            }

            if (score > bestScore) {
                bestScore = score;
                bestRow = i;
            }
        }

        s[pos] = bestRow;
        used[bestRow] = 1;
    }

    free(used);
}


/* NEIGHBORHOOD*/

static inline void transposeMove(long int *s, int i) {
    long int tmp = s[i];
    s[i] = s[i + 1];
    s[i + 1] = tmp;
}

static inline void exchangeMove(long int *s, int i, int j) {
    long int tmp = s[i];
    s[i] = s[j];
    s[j] = tmp;
}

static inline void insertMove(long int *s, int i, int j) {
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


/* DELTA EVALUATION */
/* These functions compute the change in cost for each type of move, 
it allows optimization for efficient neighborhood search */

static inline long long int deltaTranspose(long int *s, int i) {
    /*Swapping adjacent elements at positions i and i+1,
     only one pair changes the contribution:*/
    long int a = s[i];
    long int b = s[i + 1];
    return (long long int)CostMat[b][a] - (long long int)CostMat[a][b];
}


static inline long long int deltaExchange(long int *s, int i, int j) {
    /* exchanging  positions i and j, 
    the only contributions involving elements between i and j change */
    long long int delta = 0;
    long int a = s[i];
    long int b = s[j];
    int k;

    delta += (long long int)CostMat[b][a] - (long long int)CostMat[a][b];

    for (k = i + 1; k < j; k++) {
        long int x = s[k];
        delta += (long long int)CostMat[b][x]
               + (long long int)CostMat[x][a]
               - (long long int)CostMat[a][x]
               - (long long int)CostMat[x][b];
    }

    return delta;
}


static inline long long int deltaInsert(long int *s, int i, int j) {
    long long int delta = 0;
    long int a = s[i];
    int k;

    if (i < j) {
        for (k = i + 1; k <= j; k++) {
            long int x = s[k];
            delta += (long long int)CostMat[x][a] - (long long int)CostMat[a][x];
        }
    } else if (i > j) {
        for (k = j; k < i; k++) {
            long int x = s[k];
            delta += (long long int)CostMat[a][x] - (long long int)CostMat[x][a];
        }
    }

    return delta;
}


/* PIVOTING RULES */

static int firstImprovement(long int *s, Neighborhood neighborhood, long long int *currentCost) {
    int i, j;
    long long int delta;

    switch (neighborhood) {
        case TRANSPOSE:
            for (i = 0; i < PSize - 1; i++) {
                delta = deltaTranspose(s, i);
                if (delta > 0) {
                    transposeMove(s, i);
                    *currentCost += delta;
                    return 1;
                }
            }
            break;

        case EXCHANGE:
            for (i = 0; i < PSize - 1; i++) {
                for (j = i + 1; j < PSize; j++) {
                    delta = deltaExchange(s, i, j);
                    if (delta > 0) {
                        exchangeMove(s, i, j);
                        *currentCost += delta;
                        return 1;
                    }
                }
            }
            break;

        case INSERT:
            for (i = 0; i < PSize; i++) {
                for (j = 0; j < PSize; j++) {
                    if (i == j) continue;

                    delta = deltaInsert(s, i, j);
                    if (delta > 0) {
                        insertMove(s, i, j);
                        *currentCost += delta;
                        return 1;
                    }
                }
            }
            break;

        default:
            fatal("firstImprovement: invalid neighborhood.");
    }

    return 0;
}


static int bestImprovement(long int *s, Neighborhood neighborhood, long long int *currentCost) {
    int i, j;
    int bestI = -1, bestJ = -1;
    long long int delta;
    long long int bestDelta = 0;

    switch (neighborhood) {
        case TRANSPOSE:
            for (i = 0; i < PSize - 1; i++) {
                delta = deltaTranspose(s, i);
                if (delta > bestDelta) {
                    bestDelta = delta;
                    bestI = i;
                }
            }

            if (bestDelta > 0) {
                transposeMove(s, bestI);
                *currentCost += bestDelta;
                return 1;
            }
            break;

        case EXCHANGE:
            for (i = 0; i < PSize - 1; i++) {
                for (j = i + 1; j < PSize; j++) {
                    delta = deltaExchange(s, i, j);
                    if (delta > bestDelta) {
                        bestDelta = delta;
                        bestI = i;
                        bestJ = j;
                    }
                }
            }

            if (bestDelta > 0) {
                exchangeMove(s, bestI, bestJ);
                *currentCost += bestDelta;
                return 1;
            }
            break;

        case INSERT:
            for (i = 0; i < PSize; i++) {
                for (j = 0; j < PSize; j++) {
                    if (i == j) continue;

                    delta = deltaInsert(s, i, j);
                    if (delta > bestDelta) {
                        bestDelta = delta;
                        bestI = i;
                        bestJ = j;
                    }
                }
            }

            if (bestDelta > 0) {
                insertMove(s, bestI, bestJ);
                *currentCost += bestDelta;
                return 1;
            }
            break;

        default:
            fatal("bestImprovement: invalid neighborhood.");
    }

    return 0;
}


/* ITERATIVE IMPROVEMENT */

int iterativeImprovement(long int *s,
                         Neighborhood neighborhood,
                         PivotingRule pivotingRule,
                         InitialSolution initialSolution) {
    int improved;
    long long int currentCost;

    /* Initial solution */
    if (initialSolution == CHENERY_WATANABE) {
        chenery_and_watanabe(s);
    } else {
        createRandomSolution(s);
    }

    currentCost = computeCost(s);

    /* Repeat until local optimum */
    do {
        if (pivotingRule == FIRST_IMPROVEMENT) {
            improved = firstImprovement(s, neighborhood, &currentCost);
        } else if (pivotingRule == BEST_IMPROVEMENT) {
            improved = bestImprovement(s, neighborhood, &currentCost);
        } else {
            fatal("iterativeImprovement: invalid pivoting rule.");
            improved = 0;
        }
    } while (improved);

    return 0;
}


/* Variable Neighborhood Descent */

void VND(long int *s, int order) {
    int idx = 0;
    long long int currentCost;
    
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

    chenery_and_watanabe(s);
    currentCost = computeCost(s);

    while (idx < 3) {
        if (firstImprovement(s, neighborhoods[idx], &currentCost)) {
            idx = 0;
        } else {
            idx++;
        }
    }
}

/* ========== STOCHASTIC LOCAL SEARCH ALGORITHMS ========== */

static int randomImprovement(long int *s, Neighborhood neighborhood, long long int *currentCost) {
    int i, j;
    long long int delta;
    long long int bestDelta = 0;

switch (neighborhood) {

            case TRANSPOSE:
                i = rand01(&Seed) % (PSize - 1);
                delta = deltaTranspose(s, i);
                transposeMove(s, i);
                *currentCost += delta;
                return 1;

            case EXCHANGE:
                i = rand01(&Seed) % (PSize - 1);
                j = i + 1 + rand01(&Seed) % (PSize - i - 1);
                delta = deltaExchange(s, i, j);
                exchangeMove(s, i, j);
                *currentCost += delta;
                return 1;

            case INSERT:
                i = rand01(&Seed) % PSize;
                do {
                    j = rand01(&Seed) % PSize;
                } while (i == j);
                delta = deltaInsert(s, i, j);
                insertMove(s, i, j);
                *currentCost += delta;
                return 1;

            default:
                fatal("randomImprovement: invalid neighborhood.");

        }
    return 0;
}

/* Randomised Iterative Improvement */
static int randoIterativeImprovement(long int *s, Neighborhood neighborhood, PivotingRule pivotingRule,
                         InitialSolution initialSolution) {
    double wp = 0.3; // probability of accepting a non-improving move
    int step, maxSteps = 100000;   // stopping condition 
    long long int currentCost = computeCost(s);

    /* Initial solution */
    if (initialSolution == CHENERY_WATANABE) {
        chenery_and_watanabe(s);
    } else {
        createRandomSolution(s);
    }

    for (step = 0; step < maxSteps; step++) {
        if (ran01(&Seed) < wp) {
            return randomImprovement(s, neighborhood, currentCost);
        } else {
            return firstImprovement(s, neighborhood, currentCost);
        }
    }

    return 0;
}  
