/* 
  Dynamic adaptation template for HOG 

  Author: Grigori Fursin
*/


/* 

  Template function to implement decision mechanism
  for dynamic adaptation of HOG
   (can be done via decision trees, neural networks, SVM, KNN, etc)

  Based on 
   * https://scholar.google.fr/citations?view_op=view_citation&hl=en&citation_for_view=IwcnpkwAAAAJ:qjMakFHDy7sC
   * https://hal.inria.fr/hal-01054763
   * http://arxiv.org/abs/1506.06256
   * http://arxiv.org/abs/1407.4075

  input - int vector with features used to prepare an output
          (i.e. making a decision which algorithm to run)
              0 - image width
              1 - image height
              2 - image width*height
              3 - hardware species (future work - all hardware should get representative CK label, obtained via CK_HARDWARE_SPECIES environment)

  input_f - float vector with features
             0 - CPU frequency (passed via CK_CPU_FREQUENCY environment)
             1 - GPU frequency (passed via CK_GPU_FREQUENCY environment)

  output - boolean vector specifying which HOG algorithm to run:
             0 - CPU
             1 - OpenCL
             2 - PENCIL
*/

#include <stdio.h>

int adapt_hog(bool* output, int* input, float* input_f)
{
  int i=0;

  printf("\n");
  printf("*** Decision tree\n");

  printf("\n");
  printf("int feature vector:\n");

  for (i=0; i<4; i++) printf(" [%u] = %u\n", i, input[i]);

  printf("\n");
  printf("int feature vector:\n");

  for (i=0; i<2; i++) printf(" [%u] = %f\n", i, input_f[i]);

  printf("\n");

  /* Add your decision mechanism here - 
  in the future, we plan to add it as a plugin
  to be able to continuously improve predictions
  via collective knowledge - see above papers */








  return 0;
}
