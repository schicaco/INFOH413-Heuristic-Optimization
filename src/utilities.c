#include <stdio.h>
#include <stdlib.h>

#include "utilities.h"
#include "instance.h"
#include "string.h"

long int Seed;


long int  **createMatrix (long int dim) {

  int k;
  long int **result = (long int **)calloc(dim,sizeof(long int *));

  for (k=0; k<dim; ++k) {
    result[k] = (long int *)calloc(dim, sizeof(long int));
  }

  return result;
}

void fatal (char *s) {
  fprintf (stderr, "%s\n", s);
  exit(1);
}

double ran01( long *idum ) {
/* 
      FUNCTION:      returns a pseudo-random number
      INPUT:         a pointer to the seed variable 
      OUTPUT:        a pseudo-random number uniformly distributed in [0,1]
      (SIDE)EFFECTS: changes the value of seed
*/
  long k;
  double ans;

  k =(*idum)/IQ;
  *idum = IA * (*idum - k * IQ) - IR * k;
  if (*idum < 0 ) *idum += IM;
  ans = AM * (*idum);
  return ans;
}


/* Return random integer in the (inclusive) range
 * [minimum, maximum]
 */
int randInt(int minimum, int maximum) {
  return ( (int)(ran01(&Seed)*(maximum - minimum + 1)) + minimum );
}  


long int * generate_random_vector(long int dim)
/* Generates a random vector, quick and dirty */
{
  long int     *random_vector;
  int     i, help, node, tot_assigned = 0;
  double  rnd;
  
   random_vector = (long int *)malloc(dim * sizeof(long int));  

   if (!random_vector) {
     fatal("Error on random_vector malloc\n");
    }

   for ( i = 0 ; i < dim; i++) 
     random_vector[i] = i;

   for ( i = 0 ; i < dim ; i++ ) {
     /* find (randomly) an index for a free unit */ 
     rnd  = ran01 ( &Seed );
     node = (long int) (rnd  * (dim - tot_assigned)); 
     help = random_vector[i];
     random_vector[i] = random_vector[i+node];
     random_vector[i+node] = help;
     tot_assigned++;
   }

  return random_vector;
}