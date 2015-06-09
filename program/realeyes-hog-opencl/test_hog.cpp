/* FGG */
#ifdef WINDOWS
# include <numeric>
# define M_PI 3.14159265358979323846
#endif

#include "utility.hpp"
#include "HogDescriptor.h"

#include <opencv2/core/core.hpp>
#include <opencv2/ocl/ocl.hpp>

#ifdef __PENCIL__
#include <prl.h>
#include "hog.pencil.h"
#endif

#include <chrono>
#include <random>
#include <array>
#include <algorithm>
#include <cfloat>
#include <cassert>
#include <fstream>
#include <iostream>

#ifdef WITH_TBB
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#endif

//FGG
#ifdef XOPENME
#include <xopenme.h>
#endif

//FGG
#ifndef SELECT_CL_DEVICE_TYPE
#define SELECT_CL_DEVICE_TYPE CL_DEVICE_TYPE_GPU
#endif

//FGG
#ifdef HOG4X4
# define NUMBER_OF_CELLS  4 
# define NUMBER_OF_BINS   8
# define GAUSSIAN_WEIGHTS 1
# define SPARTIAL_WEIGHTS 1 
# define SIGNED_HOG       1
#endif

//FGG
#ifdef HOG2X2
# define NUMBER_OF_CELLS  2 
# define NUMBER_OF_BINS   9
# define GAUSSIAN_WEIGHTS 0
# define SPARTIAL_WEIGHTS 1 
# define SIGNED_HOG       0
#endif

//FGG
#ifdef HOG1X1
# define NUMBER_OF_CELLS  1 
# define NUMBER_OF_BINS   9
# define GAUSSIAN_WEIGHTS 1
# define SPARTIAL_WEIGHTS 0 
# define SIGNED_HOG       0
#endif

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 64
#endif

#ifndef NUMBER_OF_LOCATIONS
#define NUMBER_OF_LOCATIONS 50
#endif

#ifndef NUMBER_OF_CELLS
#define NUMBER_OF_CELLS 1
#endif

#ifndef NUMBER_OF_BINS
#define NUMBER_OF_BINS 8
#endif

#ifndef GAUSSIAN_WEIGHTS
#define GAUSSIAN_WEIGHTS 1
#endif

#ifndef SPARTIAL_WEIGHTS
#define SPARTIAL_WEIGHTS 0
#endif

#ifndef SIGNED_HOG
#define SIGNED_HOG 1
#endif

#ifndef HOG_OPENCL_CL //FGG
#define HOG_OPENCL_CL "hog/hog.opencl.cl"
#endif

namespace {
    template<typename T>
    inline int fast_floor(T f) {
        static_assert(std::is_floating_point<T>::value, "fast_floor: Parameter must be floating point.");
        return static_cast<int>(f)-(f < T(0));
    }

    template<typename T>
    inline int fast_ceil(T f) {
        static_assert(std::is_floating_point<T>::value, "fast_ceil: Parameter must be floating point.");
        return static_cast<int>(f)+(f > T(0));
    }

    inline size_t round_to_multiple(size_t num, size_t factor) {
        return num + factor - 1 - (num - 1) % factor;
    }
}

nel::HOGDescriptorCPP::HOGDescriptorCPP(int numberOfCells_, int numberOfBins_, bool gauss_, bool spinterp_, bool _signed_)
    : numberOfCells(numberOfCells_)
    , numberOfBins (numberOfBins_ )
    , gauss        (gauss_        )
    , spinterp     (spinterp_     )
    , _signed      (_signed_      )
{
    assert(numberOfCells > 1 || !spinterp);
    m_lookupTable.resize(512 * 512);
    for (int mdy = -255; mdy < 256; ++mdy)
        for (int mdx = -255; mdx < 256; ++mdx) {
            m_lookupTable[(mdy + 255) * 512 + mdx + 255].first = get_orientation(mdy,mdx);
            m_lookupTable[(mdy + 255) * 512 + mdx + 255].second = std::hypot(mdx, mdy);
        }
}

int nel::HOGDescriptorCPP::getNumberOfBins() const {
    return numberOfCells * numberOfCells * numberOfBins;
}

float nel::HOGDescriptorCPP::get_orientation(float mdy, float mdx) const {
    if (_signed) {
        return std::atan2(mdy, mdx) / static_cast<float>(M_PI) / 2.0f;
    } else {
        return std::atan2(mdy, mdx) / static_cast<float>(M_PI) + 0.5f;
    }
}

cv::Mat_<float> nel::HOGDescriptorCPP::compute( const cv::Mat_<uint8_t>  &image
                                              , const cv::Mat_<float> &locations
                                              , const cv::Mat_<float> &blocksizes
                                              ) const
{
    assert(2 == locations.cols);
    assert(2 == blocksizes.cols);
    cv::Mat_<float> descriptors(locations.rows, getNumberOfBins(), 0.0f);

#ifdef WITH_TBB
    tbb::parallel_for(tbb::blocked_range<size_t>(0, locations.rows, 5), [&](const tbb::blocked_range<size_t> range) {
    for (size_t n = range.begin(); n != range.end(); ++n) {
#else
    for (size_t n = 0; n < (size_t)locations.rows; ++n) {
#endif

        const float &blocksizeX = blocksizes(n,0);
        const float &blocksizeY = blocksizes(n,1);
        const float centerx = locations(n, 0);
        const float centery = locations(n, 1);

        const float cellsizeX = blocksizeX / numberOfCells;
        const float cellsizeY = blocksizeY / numberOfCells;
        const float halfblocksizeX = blocksizeX / 2.0f;
        const float halfblocksizeY = blocksizeY / 2.0f;

        const float minx = centerx - halfblocksizeX;
        const float miny = centery - halfblocksizeY;
        const float maxx = centerx + halfblocksizeX;
        const float maxy = centery + halfblocksizeY;

        const int minxi = std::max(fast_ceil(minx) , 1);
        const int minyi = std::max(fast_ceil(miny) , 1);
        const int maxxi = std::min(fast_floor(maxx), image.cols - 2);
        const int maxyi = std::min(fast_floor(maxy), image.rows - 2);

        cv::Mat_<float> hist(numberOfCells * numberOfCells, numberOfBins, 0.0f);

        float m1p2sigmaX2;
        float m1p2sigmaY2;
        if (gauss) {
            float sigmaX = halfblocksizeX;
            float sigmaY = halfblocksizeY;
            float sigmaX2 = sigmaX*sigmaX;
            float sigmaY2 = sigmaY*sigmaY;
            m1p2sigmaX2 = -1.0f/(2.0f*sigmaX2);
            m1p2sigmaY2 = -1.0f/(2.0f*sigmaY2);
            //float A = 1.0f / (2 * (float)M_PI*sigma2); // constant multiplier to all bin elements, but we normalize at the end, so we can skip this
        }

        // compute edges, magnitudes, orientations and the histogram
        for (int pointy = minyi; pointy <= maxyi; pointy++) {
            #ifndef __clang__
            # pragma GCC diagnostic push
            # pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
            #endif
            int cellyi;
            float yscale0;
            float yscale1;
            if (spinterp) {
                //Relative position of the pixel compared to the cell centers - y dimension
                float relative_pos_y = (pointy - miny) / cellsizeY - 0.5f;
                //Calculate the integral part and the fractional part of the relative position - y dimension
                cellyi = fast_floor(relative_pos_y);

                yscale1 = relative_pos_y - cellyi;
                yscale0 = 1.0f - yscale1;
            }

            float dy2;
            if (gauss) {
                float dy = pointy - centery;
                dy2 = dy*dy;
            }

            for (int pointx = minxi; pointx <= maxxi; pointx++) {
                int mdxi = image(pointy, pointx + 1) - image(pointy, pointx - 1);
                int mdyi = image(pointy + 1, pointx) - image(pointy - 1, pointx);

                float magnitude = m_lookupTable[(mdyi + 255) * 512 + mdxi + 255].second;
                float orientation = m_lookupTable[(mdyi + 255) * 512 + mdxi + 255].first;

                if (gauss) {
                    float dx = pointx - centerx;
                    float dx2 = dx*dx;
                    float B = std::exp(dx2 * m1p2sigmaX2 + dy2 * m1p2sigmaY2);
                    magnitude *= B;
                }

                // linear/trilinear interpolation of magnitudes
                float relative_orientation = orientation * numberOfBins - 0.5f;
                int bin1 = fast_ceil(relative_orientation);
                int bin0 = bin1 - 1;
                float magscale0 = magnitude * (bin1 - relative_orientation);
                float magscale1 = magnitude * (relative_orientation - bin0);
                int binidx0 = (bin0 + numberOfBins) % numberOfBins;
                int binidx1 = (bin1 + numberOfBins) % numberOfBins;

                if (spinterp) {
                    //Relative position of the pixel compared to the cell centers - x dimension
                    float relative_pos_x = (pointx - minx) / cellsizeX - 0.5f;
                    //Calculate the integral part and the fractional part of the relative position - x dimension
                    int cellxi = fast_floor(relative_pos_x);

                    float xscale1 = relative_pos_x - cellxi;
                    float xscale0 = 1.0f - xscale1;

                    if (cellyi >= 0                && cellxi >= 0) {
                        hist((cellyi + 0) * numberOfCells + cellxi + 0, binidx0) += yscale0 * xscale0 * magscale0;
                        hist((cellyi + 0) * numberOfCells + cellxi + 0, binidx1) += yscale0 * xscale0 * magscale1;
                    }
                    if (cellyi >= 0                && cellxi < numberOfCells - 1) {
                        hist((cellyi + 0) * numberOfCells + cellxi + 1, binidx0) += yscale0 * xscale1 * magscale0;
                        hist((cellyi + 0) * numberOfCells + cellxi + 1, binidx1) += yscale0 * xscale1 * magscale1;
                    }
                    if (cellyi < numberOfCells - 1 && cellxi >= 0) {
                        hist((cellyi + 1) * numberOfCells + cellxi + 0, binidx0) += yscale1 * xscale0 * magscale0;
                        hist((cellyi + 1) * numberOfCells + cellxi + 0, binidx1) += yscale1 * xscale0 * magscale1;
                    }
                    if (cellyi < numberOfCells - 1 && cellxi < numberOfCells - 1) {
                        hist((cellyi + 1) * numberOfCells + cellxi + 1, binidx0) += yscale1 * xscale1 * magscale0;
                        hist((cellyi + 1) * numberOfCells + cellxi + 1, binidx1) += yscale1 * xscale1 * magscale1;
                    }
                } else {
                    if (numberOfCells == 1) {
                        hist(0, binidx0) += magscale0;
                        hist(0, binidx1) += magscale1;
                    } else {
                        int cellxi = fast_floor((pointx - minx) / cellsizeX);
                        int cellyi = fast_floor((pointy - miny) / cellsizeY);
                        assert(cellxi < numberOfCells);
                        assert(cellyi < numberOfCells);
                        assert(cellxi >= 0);
                        assert(cellyi >= 0);
                        hist(cellyi * numberOfCells + cellxi, binidx0) += magscale0;
                        hist(cellyi * numberOfCells + cellxi, binidx1) += magscale1;
                    }
                }
            }
            #ifndef __clang__
            # pragma GCC diagnostic pop
            #endif
        }
        cv::Mat_<float> destRow = descriptors.row(n);
        hist.reshape(0,1).copyTo(destRow);
    }
#ifdef WITH_TBB
    });
#endif
    return descriptors;
}

nel::HOGDescriptorOCL::HOGDescriptorOCL(int numberOfCells_, int numberOfBins_, bool gauss, bool spinterp, bool _signed)
    : numberOfCells(numberOfCells_)
    , numberOfBins(numberOfBins_)
{
    assert(numberOfCells > 1 || !spinterp);

    //Get the used device
    std::vector<cl::Device> devices;
    cl::Platform::getDefault().getDevices(SELECT_CL_DEVICE_TYPE, &devices);
    device = devices.at(0);

    //FGG - print device
    std::cout << "Device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

    //Create context
    context = cl::Context(device);

    has_local_memory       = (CL_LOCAL == device.getInfo<CL_DEVICE_LOCAL_MEM_TYPE>());

    //Load source
    std::ifstream source_file{ HOG_OPENCL_CL };
    std::string source{ std::istreambuf_iterator<char>{source_file}, std::istreambuf_iterator<char>{} };

    //Create program
    program = cl::Program(context, source);

    //Build program
    std::stringstream build_opts;
    build_opts << " -cl-single-precision-constant";
    build_opts << " -cl-denorms-are-zero";
    build_opts << " -cl-no-signed-zeros";
#ifdef CL_FINITE_MATH_ONLY
    build_opts << " -cl-finite-math-only";
#endif
//    build_opts << " -cl-unsafe-math-optimizations"; //might work, but increases HOG 2x2 error over 1e-6 by a bit.
//    build_opts << " -cl-fast-relaxed-math";   //Does not work with Intel HD4000
    build_opts << " -Werror";
    build_opts << " -D NUMBER_OF_CELLS=" << numberOfCells;
    build_opts << " -D NUMBER_OF_BINS="  << numberOfBins;
    build_opts << " -D GAUSSIAN_WEIGHTS=" << (gauss    ? "1" : "0");
    build_opts << " -D SPARTIAL_WEIGHTS=" << (spinterp ? "1" : "0");
    build_opts << " -D SIGNED_HOG="       << (_signed  ? "1" : "0");
    if (!has_local_memory)
        build_opts << " -D DISABLE_LOCAL";

#ifdef XOPENME
    xopenme_add_var_s(2, (char*) "  \"kernel_build_options\":\"%s\"", (void*) build_opts.str().c_str());
#endif

    try {
        program.build(build_opts.str().c_str());
    } catch (const cl::Error&) {
        auto buildlog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        std::cerr << "OpenCL compilation error: " << buildlog << std::endl;
        throw;
    }

    //Create queue
    queue = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE);

    //Query kernels and work group infos
    calc_hog  = cl::Kernel(program, "calc_hog");
    calc_hog_preferred_multiple = calc_hog.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(device);
    calc_hog_group_size         = calc_hog.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

#ifndef CL_VERSION_1_2
    fill_zeros = cl::Kernel(program, "fill_zeros");
    fill_zeros_preferred_multiple = fill_zeros.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(device);
    fill_zeros_group_size         = fill_zeros.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);
#endif
}

int nel::HOGDescriptorOCL::getNumberOfBins() const {
    return numberOfCells * numberOfCells * numberOfBins;
}

cv::Mat_<float> nel::HOGDescriptorOCL::compute( const cv::Mat_<uint8_t> &image
                                              , const cv::Mat_<float>   &locations
                                              , const cv::Mat_<float>   &blocksizes
                                              , const size_t             max_blocksize_x
                                              , const size_t             max_blocksize_y
                                              ) const
{
    assert(2 == locations.cols);
    assert(locations.isContinuous());
    assert(2 == blocksizes.cols);
    assert(blocksizes.isContinuous());
    assert(locations.rows == blocksizes.rows);

    int num_locations = locations.rows;
    cv::Mat_<float> descriptors(num_locations, getNumberOfBins());
    assert(descriptors.isContinuous());
    
    //Events to track execution time
    cl::Event start, end;

    //OPENCL START
    //Allocate OpenCL buffers
    size_t      image_bytes = image.elemSize()*image.rows*image.step1();
    size_t  locations_bytes = sizeof(cl_float2)*num_locations;
    size_t blocksizes_bytes = sizeof(cl_float2)*blocksizes.rows;
    size_t descriptor_bytes = sizeof(cl_float)*getNumberOfBins()*num_locations;
    cl::Buffer      image_cl = cl::Buffer(context, CL_MEM_READ_ONLY,       image_bytes);
    cl::Buffer  locations_cl = cl::Buffer(context, CL_MEM_READ_ONLY,   locations_bytes);
    cl::Buffer blocksizes_cl = cl::Buffer(context, CL_MEM_READ_ONLY,  blocksizes_bytes);
    cl::Buffer descriptor_cl = cl::Buffer(context, CL_MEM_READ_WRITE, descriptor_bytes);

    //Write input buffers to device
    queue.enqueueWriteBuffer(     image_cl, CL_FALSE, 0,      image_bytes,      image.data);
    queue.enqueueWriteBuffer( locations_cl, CL_FALSE, 0,  locations_bytes,  locations.data);
    queue.enqueueWriteBuffer(blocksizes_cl, CL_FALSE, 0, blocksizes_bytes, blocksizes.data);

#ifdef CL_VERSION_1_2
    queue.enqueueFillBuffer(descriptor_cl, 0.0f, 0, descriptor_bytes, nullptr, &start);
#else
    {
        cl::NDRange  local_work_size(fill_zeros_preferred_multiple);
        cl::NDRange global_work_size( round_to_multiple(getNumberOfBins()*num_locations, fill_zeros_preferred_multiple) );
        {
            //ADD MULTITHREAD LOCK HERE (if needed)
            fill_zeros.setArg(0, descriptor_cl());
            fill_zeros.setArg(1, (cl_int)(getNumberOfBins()*num_locations));
            queue.enqueueNDRangeKernel(fill_zeros, cl::NullRange, global_work_size, local_work_size, nullptr, &start );
        }
    }
#endif

#ifndef LWS_X
#define LWS_X calc_hog_preferred_multiple
#endif

#ifndef LWS_Y
#define LWS_Y std::max(calc_hog_group_size / num_locations / LWS_X, (size_t)1u)
#endif

#ifndef LWS_Z
#define LWS_Z std::max(calc_hog_group_size / LWS_Y / LWS_X, (size_t)1u)
#endif

#define DEFAULT_LWS_X calc_hog_preferred_multiple
#define DEFAULT_LWS_Y std::max(calc_hog_group_size / num_locations / LWS_X, (size_t)1u)
#define DEFAULT_LWS_Z std::max(calc_hog_group_size / LWS_Y / LWS_X, (size_t)1u)


#ifdef XOPENME
    xopenme_add_var_i(3, (char*) "  \"default_lws_x\":%u", DEFAULT_LWS_X);
    xopenme_add_var_i(4, (char*) "  \"default_lws_y\":%u", DEFAULT_LWS_Y);
    xopenme_add_var_i(5, (char*) "  \"default_lws_z\":%u", DEFAULT_LWS_Z);
#endif

    {

       //Figure out the work group sizes
        const size_t lws_x = LWS_X;
        const size_t lws_y = LWS_Y;
        const size_t lws_z = LWS_Z;
        cl::NDRange local_work_size(lws_x, lws_y, lws_z);

        const size_t gws_x = round_to_multiple(max_blocksize_x, lws_x);
        const size_t gws_y = round_to_multiple(max_blocksize_y, lws_y);
        const size_t gws_z = round_to_multiple(num_locations,   lws_z);
        cl::NDRange global_work_size(gws_x, gws_y, gws_z);

        //Execute the kernel
        {
            //ADD MULTITHREAD LOCK HERE (if needed)
            calc_hog.setArg(0, (cl_int)image.rows);
            calc_hog.setArg(1, (cl_int)image.cols);
            calc_hog.setArg(2, (cl_int)image.step1());
            calc_hog.setArg(3, image_cl());
            calc_hog.setArg(4, (cl_int)num_locations);
            calc_hog.setArg(5, locations_cl());
            calc_hog.setArg(6, blocksizes_cl());
            calc_hog.setArg(7, descriptor_cl());
            if (has_local_memory)
                calc_hog.setArg(8, (size_t)(sizeof(cl_float) * getNumberOfBins() * lws_z), nullptr);
            queue.enqueueNDRangeKernel(calc_hog, cl::NullRange, global_work_size, local_work_size, nullptr, &end );
        }
    }

    //Read result buffer from device
    queue.enqueueReadBuffer(descriptor_cl, CL_TRUE, 0, descriptor_bytes, descriptors.data);

    //Calculate kernel-only execution time
    double kernel_ns = end.getProfilingInfo<CL_PROFILING_COMMAND_END>() - start.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    double kernel_ms = kernel_ns * 1e-6;
    std::cout << "calc_hog execution time: " << std::fixed << std::setprecision(6) << std::setw(8) << kernel_ms << " ms\n";

    //Normalize result
//    for (int n = 0; n < num_locations; ++n) {
//        float scale = static_cast<float>(cv::norm(descriptors.row(n), cv::NORM_L2));
//        if (scale < std::numeric_limits<float>::epsilon())
//            descriptors.row(n)= 0.0f;
//        else {
//            auto scaled = descriptors.row(n)/scale;
//            scaled = cv::min(scaled, 0.3f);
//            cv::normalize(scaled, descriptors.row(n));
//        }
//    }
    return descriptors;
}

void time_hog( const std::vector<carp::record_t>& pool, const std::vector<float>& sizes, int num_positions, int repeat )
{
    carp::Timing timing("HOG");
    bool first_execution_opencl = true;
#ifdef __PENCIL__
    bool first_execution_pencil = true;
#endif

    for (;repeat>0; --repeat) {
        for ( auto & size : sizes ) {
            for ( auto & item : pool ) {
                std::mt19937 rng(1);   //uses same seed, reseed for all iteration

                const char* item_path=(item.path()).c_str();

                std::cout << "image path: " << item_path << std::endl;

                cv::Mat cpu_gray, orig;

                orig=item.cpuimg();
                if (orig.empty()){
                  std::cerr << "  empty image..." << std::endl;
                  break;
                }
                else if (orig.channels()>1) cv::cvtColor( orig, cpu_gray, CV_RGB2GRAY );
                else cpu_gray=orig;

                std::cout << "image rows: " << cpu_gray.rows << std::endl;
                std::cout << "image cols: " << cpu_gray.cols << std::endl;

#ifdef XOPENME
	        xopenme_add_var_i(0, (char*) "  \"input_size_x\":%u", cpu_gray.rows);
	        xopenme_add_var_i(1, (char*) "  \"input_size_y\":%u", cpu_gray.cols);
#endif

                cv::Mat_<float> locations(num_positions, 2);
                cv::Mat_<float> blocksizes(num_positions, 2);
                size_t max_blocksize_x = std::ceil(size);
                size_t max_blocksize_y = std::ceil(size);
                //fill locations and blocksizes
                std::uniform_real_distribution<float> genx(size/2+1, cpu_gray.rows-1-size/2-1);
                std::uniform_real_distribution<float> geny(size/2+1, cpu_gray.cols-1-size/2-1);
                for( int i = 0; i < num_positions; ++i) {
                    locations(i, 0) = genx(rng);
                    locations(i, 1) = geny(rng);
                    blocksizes(i, 0) = size;
                    blocksizes(i, 1) = size;
                }

                cv::Mat_<float> cpu_result, gpu_result, pen_result;
                std::chrono::duration<double> elapsed_time_cpu, elapsed_time_gpu;

                {
                    //CPU implementation
                    static nel::HOGDescriptorCPP descriptor( NUMBER_OF_CELLS
                                                           , NUMBER_OF_BINS
                                                           , GAUSSIAN_WEIGHTS
                                                           , SPARTIAL_WEIGHTS
                                                           , SIGNED_HOG
                                                           );
                    const auto cpu_start = std::chrono::high_resolution_clock::now();
                    cpu_result = descriptor.compute(cpu_gray, locations, blocksizes);
                    const auto cpu_end = std::chrono::high_resolution_clock::now();

                    elapsed_time_cpu = cpu_end - cpu_start;
                    //Free up resources
                }
                {
                    //OpenCL implementation
                    static nel::HOGDescriptorOCL descriptor( NUMBER_OF_CELLS
                                                           , NUMBER_OF_BINS
                                                           , GAUSSIAN_WEIGHTS
                                                           , SPARTIAL_WEIGHTS
                                                           , SIGNED_HOG
                                                           );

		    //First execution includes buffer allocation
	            if (first_execution_opencl)
		    {
	                 gpu_result = descriptor.compute(cpu_gray, locations, blocksizes, max_blocksize_x, max_blocksize_y);
		         first_execution_opencl = false;
		    }

                    const auto gpu_start = std::chrono::high_resolution_clock::now();
                    gpu_result = descriptor.compute(cpu_gray, locations, blocksizes, max_blocksize_x, max_blocksize_y);
                    const auto gpu_end = std::chrono::high_resolution_clock::now();

                    elapsed_time_gpu = gpu_end - gpu_start;
                    //Free up resources
                }
#ifdef __PENCIL__
                {
		    //PENCIL implementation
                    pen_result.create(num_positions, HISTOGRAM_BINS);

		    if (first_execution_pencil)
                    {
		         pencil_hog( NUMBER_OF_CELLS, NUMBER_OF_BINS, GAUSSIAN_WEIGHTS, SPARTIAL_WEIGHTS, SIGNED_HOG
                              , cpu_gray.rows, cpu_gray.cols, cpu_gray.step1(), cpu_gray.ptr<uint8_t>()
                              , num_positions
                              , reinterpret_cast<const float (*)[2]>(locations.data)
                              , reinterpret_cast<const float (*)[2]>(blocksizes.data)
                              , reinterpret_cast<      float  *    >(pen_result.data)
                              );
			 first_execution_pencil = false;
		    }

		    prl_timings_reset();
		    prl_timings_start();

                    pencil_hog( NUMBER_OF_CELLS, NUMBER_OF_BINS, GAUSSIAN_WEIGHTS, SPARTIAL_WEIGHTS, SIGNED_HOG
                              , cpu_gray.rows, cpu_gray.cols, cpu_gray.step1(), cpu_gray.ptr<uint8_t>()
                              , num_positions
                              , reinterpret_cast<const float (*)[2]>(locations.data)
                              , reinterpret_cast<const float (*)[2]>(blocksizes.data)
                              , reinterpret_cast<      float  *    >(pen_result.data)
                              );

		    prl_timings_stop();
                    // Dump execution times for PENCIL code.
                    prl_timings_dump();
                }
#endif

/* FGG */
#ifdef XOPENME_DUMP_IMAGES
 #ifdef WINDOWS
                IplImage tmp;
                tmp=cpu_result;
                cvSaveImage("tmp-ck-state-hog-cpu.png",&tmp);
                tmp=gpu_result;
                cvSaveImage("tmp-ck-state-hog-gpu.png",&tmp);
 #else
                cv::imwrite("tmp-ck-state-hog-cpu.png", cpu_result );
                cv::imwrite("tmp-ck-state-hog-gpu.png", gpu_result );
                cv::imwrite("tmp-ck-state-hog-diff-cpu-gpu.png", cv::abs(cpu_result-gpu_result) );
 #endif
#endif

                // Verifying the results
                if ( cv::norm( cpu_result, gpu_result, cv::NORM_INF) > cv::norm( gpu_result, cv::NORM_INF)*1e-5 )
                {
                    std::cerr << "ERROR: Results don't match. Writing calculated images." << std::endl;
                    std::cerr << "CPU norm:" << cv::norm(cpu_result, cv::NORM_INF) << std::endl;
                    std::cerr << "GPU norm:" << cv::norm(gpu_result, cv::NORM_INF) << std::endl;
                    std::cerr << "GPU-CPU norm:" << cv::norm(gpu_result, cpu_result, cv::NORM_INF) << std::endl;
                    cv::imwrite( "hog_cpu.png", cpu_result );
                    cv::imwrite( "hog_gpu.png", gpu_result );
                    cv::imwrite( "hog_diff_cpu_gpu.png", cv::abs(cpu_result-gpu_result) );
                    throw std::runtime_error("The OpenCL results are not equivalent with the C++ results.");
                }
#ifdef __PENCIL__
                if ( cv::norm( cpu_result, pen_result, cv::NORM_INF) > cv::norm( cpu_result, cv::NORM_INF)*1e-5 )
                {
                    std::cerr << "ERROR: Results don't match. Writing calculated images." << std::endl;
                    std::cerr << "CPU norm:" << cv::norm(cpu_result, cv::NORM_INF) << std::endl;
                    std::cerr << "PEN norm:" << cv::norm(pen_result, cv::NORM_INF) << std::endl;
                    std::cerr << "PEN-CPU norm:" << cv::norm(pen_result, cpu_result, cv::NORM_INF) << std::endl;
                    cv::imwrite( "hog_cpu.png", cpu_result );
                    cv::imwrite( "hog_pen.png", pen_result );
                    cv::imwrite( "hog_diff_cpu_pen.png", cv::abs(cpu_result-pen_result) );
                    throw std::runtime_error("The PENCIL results are not equivalent with the C++ results.");
		}
#endif
                timing.print(elapsed_time_cpu, elapsed_time_gpu);
            }
        }
    }
}

int main(int argc, char* argv[])
{
#ifdef XOPENME
        xopenme_init(1,8);
        xopenme_clock_start(0);
#endif

    try {
#ifdef __PENCIL__
        prl_init((prl_init_flags)(PRL_TARGET_DEVICE_DYNAMIC | PRL_PROFILING_ENABLED));
#endif

	std::cout << "This executable is iterating over all the files passed to it as an argument. " << std::endl;

	auto pool = carp::get_pool(argc, argv);

#ifdef RUN_ONLY_ONE_EXPERIMENT
        time_hog( pool, {BLOCK_SIZE}, NUMBER_OF_LOCATIONS, 1 );
#else
        time_hog( pool, {16, 32, 64, 128, 192}, NUMBER_OF_LOCATIONS, 6 );
#endif

#ifdef __PENCIL__
        prl_shutdown();
#endif
    } catch(const std::exception& e) {
        std::cout << e.what() << std::endl;

#ifdef __PENCIL__
        prl_shutdown();
#endif

#ifdef XOPENME
        xopenme_clock_end(0);
        xopenme_dump_state();
        xopenme_finish();
#endif

        return EXIT_FAILURE;
    }

#ifdef XOPENME
        xopenme_clock_end(0);
        xopenme_dump_state();
        xopenme_finish();
#endif

    return EXIT_SUCCESS;

} // main
