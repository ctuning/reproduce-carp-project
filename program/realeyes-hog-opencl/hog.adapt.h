/* 
  Dynamic adaptation routine for HOG 

  (C)opyright, Grigori Fursin, 2015

  Based on 
  * https://scholar.google.fr/citations?view_op=view_citation&hl=en&citation_for_view=IwcnpkwAAAAJ:qjMakFHDy7sC
  * https://hal.inria.fr/hal-01054763
  * http://arxiv.org/abs/1506.06256
  * http://arxiv.org/abs/1407.4075

*/

#ifndef HOG_ADAPT_H
#define HOG_ADAPT_H

int adapt_hog(bool* output, int* input, float* input_f);

#endif
