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

  output - boolean vector specifying which HOG algorithm to run:
             0 - CPU
             1 - OpenCL
             2 - PENCIL
*/

int adapt_hog(int* input, bool* output)
{

  /* Add your decision mechanism here - 
  in the future, we plan to add it as a plugin
  to be able to continuously improve predictions
  via collective knowledge - see above papers */




  return 0;
}
