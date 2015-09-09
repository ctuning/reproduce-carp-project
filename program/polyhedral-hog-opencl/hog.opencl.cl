#ifndef NUMBER_OF_CELLS
#error NUMBER_OF_CELLS not defined
#endif
#ifndef NUMBER_OF_BINS
#error NUMBER_OF_BINS not defined
#endif
#ifndef GAUSSIAN_WEIGHTS
#error GAUSSIAN_WEIGHTS not defined
#endif
#ifndef SPARTIAL_WEIGHTS
#error SPARTIAL_WEIGHTS not defined
#endif
#ifndef SIGNED_HOG
#error SIGNED_HOG not defined
#endif

#define TOTAL_NUMBER_OF_BINS (NUMBER_OF_CELLS * NUMBER_OF_CELLS * NUMBER_OF_BINS)

//get_global_id(0): per location
//get_global_id(1): x coordinate
//get_global_id(2): y coordinate

inline void atomic_add_float_global(volatile __global float* source, float operand) {
    volatile __global int* sourceAsInt = (volatile __global int*)source;
    int oldVal;
    int oldVal_2 = atomic_add(sourceAsInt, 0);
    do {
        oldVal = oldVal_2;
        float newVal = as_float(oldVal) + operand;
        oldVal_2 = atomic_cmpxchg(sourceAsInt, oldVal, as_int(newVal));
    } while (oldVal_2 != oldVal);
}

#ifndef DISABLE_LOCAL
inline void atomic_add_float_local(volatile __local float* source, float operand) {
    volatile __local int* sourceAsInt = (volatile __local int*)source;
    int oldVal;
    int oldVal_2 = atomic_add(sourceAsInt, 0);
    do {
        oldVal = oldVal_2;
        float newVal = as_float(oldVal) + operand;
        oldVal_2 = atomic_cmpxchg(sourceAsInt, oldVal, as_int(newVal));
    } while (oldVal_2 != oldVal);
}
#endif

#if __OPENCL_C_VERSION__ < 120
// OpenCL 1.1 does not have clEnqueueFillBuffer...
__kernel void fill_zeros(__global float * restrict arr, int size) {
    int i = get_global_id(0);
    if (i < size) {
        arr[i] = 0.0f;
    }
}
#endif

__kernel void calc_hog( const int rows, const int cols, const int step_, __global const unsigned char * restrict image
                      , const int num_locations, __global const float2 * restrict location_global, __global const float2 * restrict blocksize_global
                      , __global float hist_global[][NUMBER_OF_CELLS][NUMBER_OF_CELLS][NUMBER_OF_BINS]
#ifndef DISABLE_LOCAL
                      ,  __local float  hist_local[][NUMBER_OF_CELLS][NUMBER_OF_CELLS][NUMBER_OF_BINS]
#endif
                      )
{
    size_t location_global_idx = get_global_id(2);
#ifdef DISABLE_LOCAL
    #define atomic_add_float_local atomic_add_float_global
    #define hist_local             hist_global
    #define location_local_idx     location_global_idx
#else
    //Clear hist_local
    size_t location_local_idx = get_local_id(2);

    //Clear hist_local
    for(int celly = get_local_id(1); celly < NUMBER_OF_CELLS; celly+=get_local_size(1)) {
        for(int cellx = get_local_id(0); cellx < NUMBER_OF_CELLS; cellx+=get_local_size(0)) {
            for(int bin = 0; bin < NUMBER_OF_BINS ; ++bin) {
                hist_local[location_local_idx][celly][cellx][bin] = 0.0f;
            }
        }
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
#endif
    
    if (location_global_idx < num_locations) {
        float2 location = location_global[location_global_idx];
        float2 blck_size = blocksize_global[location_global_idx];
        int2 img_size = (int2)(cols, rows);

        float2 minloc = location - blck_size * 0.5f;
        float2 maxloc = location + blck_size * 0.5f;

        int2 mini = max(convert_int2_rtp(minloc), 1);
        int2 maxi = min(convert_int2_rtn(maxloc), img_size - 2);

        int2 point = mini + (int2)(get_global_id(0),get_global_id(1));

        if (all(point <= maxi)) {
            //Read the image
            float mdx = image[(point.y  )*step_ + point.x+1] - image[(point.y  )*step_ + point.x-1];
            float mdy = image[(point.y+1)*step_ + point.x  ] - image[(point.y-1)*step_ + point.x  ];

            //calculate the magnitude
            float magnitude = hypot(mdx, mdy);

            //calculate the orientation
#if SIGNED_HOG
            float orientation = atan2pi(mdy, mdx) * 0.5f;
#else
            float orientation = atan2pi(mdy, mdx) + 0.5f;
#endif

#if GAUSSIAN_WEIGHTS
            {
                //Code before optimization:
                //float2 sigma = blck_size / 2.0f;
                //float2 sigmaSq = sigma*sigma;
                //float2 m1p2sigmaSq = -1.0f / (2.0f * sigmaSq);
                float2 m1p2sigmaSq = -2.0f / (blck_size * blck_size);
                float2 distance = convert_float2(point) - location;
                float2 distanceSq = distance*distance;
                magnitude *= exp(dot(distanceSq, m1p2sigmaSq));
            }
#endif
            float relative_orientation = orientation * NUMBER_OF_BINS - 0.5f;
            
            int   bin0        = convert_int_rtn(relative_orientation);
            float bin_weight1 = relative_orientation - convert_float(bin0);
            
            int   bin1        = 1    + bin0;
            float bin_weight0 = 1.0f - bin_weight1;
            
            int2   bin        = ((int2)(bin0, bin1) + NUMBER_OF_BINS) % NUMBER_OF_BINS;
            float2 bin_weight = magnitude * (float2)(bin_weight0, bin_weight1);

#if NUMBER_OF_CELLS == 1
            atomic_add_float_local( &(hist_local[location_local_idx][0][0][bin.s0]), bin_weight.s0);
            atomic_add_float_local( &(hist_local[location_local_idx][0][0][bin.s1]), bin_weight.s1);
#elif SPARTIAL_WEIGHTS
            float2 relative_pos = (convert_float2(point) - minloc) * NUMBER_OF_CELLS / blck_size - 0.5f;
            int2 celli = convert_int2_rtn(relative_pos);

            float2 scale1 = relative_pos - convert_float2(celli);
            float2 scale0 = 1.0f - scale1;

            if (celli.y >= 0 && celli.x >= 0) {
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y  ][celli.x  ][bin.s0]), scale0.y * scale0.x * bin_weight.s0);
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y  ][celli.x  ][bin.s1]), scale0.y * scale0.x * bin_weight.s1);
            }
            if (celli.y >= 0 && celli.x < NUMBER_OF_CELLS - 1) {
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y  ][celli.x+1][bin.s0]), scale0.y * scale1.x * bin_weight.s0);
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y  ][celli.x+1][bin.s1]), scale0.y * scale1.x * bin_weight.s1);
            }
            if (celli.y < NUMBER_OF_CELLS - 1 && celli.x >= 0) {
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y+1][celli.x  ][bin.s0]), scale1.y * scale0.x * bin_weight.s0);
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y+1][celli.x  ][bin.s1]), scale1.y * scale0.x * bin_weight.s1);
            }
            if (celli.y < NUMBER_OF_CELLS - 1 && celli.x < NUMBER_OF_CELLS - 1) {
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y+1][celli.x+1][bin.s0]), scale1.y * scale1.x * bin_weight.s0);
                atomic_add_float_local( &(hist_local[location_local_idx][celli.y+1][celli.x+1][bin.s1]), scale1.y * scale1.x * bin_weight.s1);
            }
#else
            int2 celli = convert_int2_rtn((convert_float2(point) - minloc) * NUMBER_OF_CELLS / blck_size);

            atomic_add_float_local( &(hist_local[location_local_idx][celli.y][celli.x][bin.s0]), bin_weight.s0);
            atomic_add_float_local( &(hist_local[location_local_idx][celli.y][celli.x][bin.s1]), bin_weight.s1);
#endif
        }
    }
#ifndef DISABLE_LOCAL
    barrier(CLK_LOCAL_MEM_FENCE);
    
    //Add local version to global
    for(int celly = get_local_id(1); celly < NUMBER_OF_CELLS; celly+=get_local_size(1)) {
        for(int cellx = get_local_id(0); cellx < NUMBER_OF_CELLS; cellx+=get_local_size(0)) {
            for(int bin = 0; bin < NUMBER_OF_BINS ; ++bin) {
                atomic_add_float_global( &(hist_global[location_global_idx][celly][cellx][bin]), hist_local[location_local_idx][celly][cellx][bin] );
            }
        }
    }
#endif
}
