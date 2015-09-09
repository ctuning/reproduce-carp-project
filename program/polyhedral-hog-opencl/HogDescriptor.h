/* ---------------------------------------------------------------------
* Copyright Realeyes OU 2012-2014
*
* All rights reserved. Realeyes OU PROPRIETARY/CONFIDENTIAL.
* No part of this computer programs(s) may be used, reproduced,
* stored in any retrieval system, or transmitted, in any form or
* by any means, electronic, mechanical, photocopying,
* recording, or otherwise without prior written permission of
* Realeyes OU. Use is subject to license terms.
* ---------------------------------------------------------------------
*/

#ifndef HOGDESCRIPTOR_H
#define HOGDESCRIPTOR_H

#include <opencv2/core/core.hpp>

#define NOMINMAX
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>

#include <vector>
#include <chrono>

namespace nel {
    class HOGDescriptorCPP {
    public:
        HOGDescriptorCPP(int numberOfCells, int numberOfBins, bool gauss, bool spinterp, bool _signed);
        cv::Mat_<float> compute( const cv::Mat_<uint8_t> &img
                               , const cv::Mat_<float>   &locations
                               , const cv::Mat_<float>   &blocksizes
                               ) const;

        int getNumberOfBins() const;

    private:
        float get_orientation(float mdy, float mdx) const;

    private:
        std::vector<std::pair<float, float> > m_lookupTable;
        int numberOfCells;
        int numberOfBins;
        bool gauss;
        bool spinterp;
        bool _signed;
    };

	class HOGDescriptorOCL {
    public:
        HOGDescriptorOCL(int numberOfCells, int numberOfBins, bool gauss, bool spinterp, bool _signed);

        cv::Mat_<float> compute( const cv::Mat_<uint8_t>       &img
                               , const cv::Mat_<float>         &locations
                               , const cv::Mat_<float>         &blocksizes
                               , const size_t                   max_blocksize_x
                               , const size_t                   max_blocksize_y
                               ) const;

        int getNumberOfBins() const;

    private:
        int numberOfCells;
        int numberOfBins;

        cl::Device device;
        cl::Context context;
        cl::Program program;
        cl::CommandQueue queue;

        mutable cl::Kernel calc_hog;
        size_t calc_hog_preferred_multiple;
        size_t calc_hog_group_size;

#ifndef CL_VERSION_1_2
        mutable cl::Kernel fill_zeros;
        size_t fill_zeros_preferred_multiple;
        size_t fill_zeros_group_size;
#endif
        bool has_local_memory;
    };
}

#endif
