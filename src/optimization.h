#ifndef _LO_H_
#define _LO_H_

extern long int **CostMat;

typedef enum { TRANSPOSE, EXCHANGE, INSERT } Neighborhood;
typedef enum { FIRST_IMPROVEMENT, BEST_IMPROVEMENT } PivotingRule;
typedef enum {RANDOM, CHENERY_WATANABE} InitialSolution;

long long int computeCost ( long int *lo );
void createRandomSolution(long int *s);
void chenery_and_watanabe(long int *s);
void VND(long int *s, int order);

int iterativeImprovement(long int *s, Neighborhood neighborhood, PivotingRule pivotingRule, InitialSolution initialSolution);

#endif