#include "hog.pencil.h"

#include <pencil.h>

static void hog_multi( const int NUMBER_OF_CELLS
                     , const int NUMBER_OF_BINS
                     , const int gauss
                     , const int spinterp
                     , const int _signed
                     , const int rows
                     , const int cols
                     , const int step
                     , const uint8_t image[static const restrict rows][step]
                     , const int num_locations
                     , const float location[static const restrict num_locations][2]
                     , const float blck_size[static const restrict num_locations][2]
                     , float hist[static const restrict num_locations][NUMBER_OF_CELLS][NUMBER_OF_CELLS][NUMBER_OF_BINS]    //out
                     ) {
#pragma scop
    __pencil_assume(rows > 3);
    __pencil_assume(cols > 3);
    __pencil_assume(cols <= step);
    __pencil_assume(NUMBER_OF_CELLS > 0);
    __pencil_assume(NUMBER_OF_BINS  > 0);
    __pencil_assume(num_locations   > 0);

    __pencil_assume(gauss    >= 0);
    __pencil_assume(spinterp >= 0);
    __pencil_assume(_signed  >= 0);

    __pencil_kill(hist);

    for (int i = 0; i < num_locations; ++i)
        for (int j = 0; j < NUMBER_OF_CELLS; ++j)
            for(int k = 0; k < NUMBER_OF_CELLS; ++k)
                for(int l = 0; l < NUMBER_OF_BINS; ++l)
                    hist[i][j][k][l] = 0;

    #pragma pencil independent
    for (int i = 0; i < num_locations; ++i) {
        float locationx = location[i][0];
        float locationy = location[i][1];
        float blck_sizex = blck_size[i][0];
        float blck_sizey = blck_size[i][1];
        float minx = locationx - blck_sizex / 2.0f;
        float miny = locationy - blck_sizey / 2.0f;
        float maxx = locationx + blck_sizex / 2.0f;
        float maxy = locationy + blck_sizey / 2.0f;

        int minxi = imax((int)ceilf(minx), 1);
        int minyi = imax((int)ceilf(miny), 1);
        int maxxi = imin((int)floorf(maxx), cols - 2);
        int maxyi = imin((int)floorf(maxy), rows - 2);

        #pragma pencil independent reduction(+:hist[i])
        for (int pointy = minyi; pointy <= maxyi; ++pointy) {
            #pragma pencil independent reduction(+:hist[i])
            for (int pointx = minxi; pointx <= maxxi; ++pointx) {
                //Read the image
                int temp1 = pointx-1;
                int temp2 = pointy-1;
                float mdx = image[pointy][pointx+1] - image[pointy][temp1];
                float mdy = image[pointy+1][pointx] - image[temp2][pointx];
                
                //calculate the magnitude
                float magnitude = hypotf(mdx, mdy);

                //calculate the orientation
                float orientation;
                if (_signed) {
                    orientation = atan2pif(mdy, mdx) / 2.0f;
                } else {
                    orientation = atan2pif(mdy, mdx) + 0.5f;
                }

                if (gauss) {
                    float sigmax = blck_sizex / 2.0f;
                    float sigmay = blck_sizey / 2.0f;
                    float sigmaSqx = sigmax*sigmax;
                    float sigmaSqy = sigmay*sigmay;
                    float m1p2sigmaSqx = -1.0f / (2.0f * sigmaSqx);
                    float m1p2sigmaSqy = -1.0f / (2.0f * sigmaSqy);
                    float distancex = (float)(pointx) - locationx;
                    float distancey = (float)(pointy) - locationy;
                    float distanceSqx = distancex * distancex;
                    float distanceSqy = distancey * distancey;
                    magnitude *= expf(distanceSqx * m1p2sigmaSqx + distanceSqy * m1p2sigmaSqy);
                }

                float relative_orientation = orientation * NUMBER_OF_BINS - 0.5f;
                int bin1 = ceilf(relative_orientation);
                int bin0 = bin1 - 1;
                float bin_weight0 = magnitude * (bin1 - relative_orientation);
                float bin_weight1 = magnitude * (relative_orientation - bin0);
                bin0 = (bin0 + NUMBER_OF_BINS) % NUMBER_OF_BINS;
                bin1 = (bin1 + NUMBER_OF_BINS) % NUMBER_OF_BINS;
		
		int cellxi;
		int cellyi;

                if (spinterp) {
                    float relative_posx = (pointx - minx) * NUMBER_OF_CELLS / blck_sizex - 0.5f;
                    float relative_posy = (pointy - miny) * NUMBER_OF_CELLS / blck_sizey - 0.5f;
                    cellxi = (int)floorf(relative_posx);
                    cellyi = (int)floorf(relative_posy);

                    float xscale1 = relative_posx - (float)(cellxi);
                    float yscale1 = relative_posy - (float)(cellyi);
                    float xscale0 = 1.0f - xscale1;
                    float yscale0 = 1.0f - yscale1;

//                    __pencil_assume(cellxi < NUMBER_OF_CELLS);
  //                  __pencil_assume(cellyi < NUMBER_OF_CELLS);
    //                __pencil_assume(cellxi >= 0);
      //              __pencil_assume(cellyi >= 0);
                    if (cellyi >= 0 && cellxi >= 0) {
                        hist[i][cellyi  ][cellxi  ][bin0] += yscale0 * xscale0 * bin_weight0;
                        hist[i][cellyi  ][cellxi  ][bin1] += yscale0 * xscale0 * bin_weight1;
                    }
                    if (cellyi >= 0 && cellxi < NUMBER_OF_CELLS - 1) {
                        hist[i][cellyi  ][cellxi+1][bin0] += yscale0 * xscale1 * bin_weight0;
                        hist[i][cellyi  ][cellxi+1][bin1] += yscale0 * xscale1 * bin_weight1;
                    }
                    if (cellyi < NUMBER_OF_CELLS - 1 && cellxi >= 0) {
                        hist[i][cellyi+1][cellxi  ][bin0] += yscale1 * xscale0 * bin_weight0;
                        hist[i][cellyi+1][cellxi  ][bin1] += yscale1 * xscale0 * bin_weight1;
                    }
                    if (cellyi < NUMBER_OF_CELLS - 1 && cellxi < NUMBER_OF_CELLS - 1) {
                        hist[i][cellyi+1][cellxi+1][bin0] += yscale1 * xscale1 * bin_weight0;
                        hist[i][cellyi+1][cellxi+1][bin1] += yscale1 * xscale1 * bin_weight1;
                    }
                } else if (NUMBER_OF_CELLS == 1) {
                    hist[i][0][0][bin0] += bin_weight0;
                    hist[i][0][0][bin1] += bin_weight1;
                } else {
                    cellxi = (int)floorf((pointx - minx) * NUMBER_OF_CELLS / blck_sizex);
                    cellyi = (int)floorf((pointy - miny) * NUMBER_OF_CELLS / blck_sizey);

        //            __pencil_assume(cellxi < NUMBER_OF_CELLS);
          //          __pencil_assume(cellyi < NUMBER_OF_CELLS);
            //        __pencil_assume(cellxi >= 0);
              //      __pencil_assume(cellyi >= 0);
                    hist[i][cellyi][cellxi][bin0] += bin_weight0;
                    hist[i][cellyi][cellxi][bin1] += bin_weight1;
                }
            }
        }
    }
    __pencil_kill(location);
    __pencil_kill(image);
#pragma endscop
}

void pencil_hog( const int NUMBER_OF_CELLS
               , const int NUMBER_OF_BINS
               , const bool gauss
               , const bool spinterp
               , const bool _signed
               , const int rows
               , const int cols
               , const int step
               , const uint8_t image[]
               , const int num_locations
               , const float location[][2]
               , const float blck_size[][2]
               , float hist[]    //out
               )
{
    hog_multi( NUMBER_OF_CELLS, NUMBER_OF_BINS, gauss, spinterp, _signed
             , rows, cols, step, (const uint8_t(*)[step])image
             , num_locations, (const float(*)[2])location
             , blck_size
             , (float(*)[NUMBER_OF_CELLS][NUMBER_OF_CELLS][NUMBER_OF_BINS])hist
             );
}

