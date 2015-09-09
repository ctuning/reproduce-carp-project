// UjoImro, 2013
// Experimental Research Code for the CARP Project

#ifndef __CARP__UTILITY__HPP__
#define __CARP__UTILITY__HPP__

#include <chrono>
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>

namespace carp {

class Timing
{
    std::vector<double> cpu_timings;
    std::vector<double> gpu_timings;
public:
    Timing(const std::string & name) {
        std::cout << "Measuring performance of " << name << std::endl;
    }

    void print( const std::chrono::duration<double,std::milli> &cpu, const std::chrono::duration<double,std::milli> &gpu ) {
        std::cout << std::fixed << std::setprecision(6);
        std::cout << std::endl << "OpenCV time measurements for an individual experiment:" << std::endl;
        std::cout << "CPU Time    - GPU+copy" << std::endl;
        std::cout << std::setw(8) << cpu.count() << " ms - ";
        std::cout << std::setw(8) << gpu.count() << " ms" << std::endl;

        cpu_timings.push_back(cpu.count());
        gpu_timings.push_back(gpu.count());
    }

    ~Timing() {
        std::cout << std::endl << "OpenCV accumulated time measurements for all the experiments (in ms):" << std::endl;
        std::cout<<"[RealEyes] Accumulate CPU time           : "<< std::accumulate(cpu_timings.begin(),cpu_timings.end(),0.0) << "\n";
        std::cout<<"[RealEyes] Accumulate GPU time (inc copy): "<< std::accumulate(gpu_timings.begin(),gpu_timings.end(),0.0) << "\n";
    }
};

class record_t {
private:
    char *m_path;

public:
    cv::Mat cpuimg() const {
//FGG
#ifdef WINDOWS
        return cvLoadImage(m_path);
#else
        return cv::imread(m_path);
#endif
    }

    std::string path() const {
        return std::string(m_path);
    }

    record_t( char *path )
    {
	m_path = path;
    }

};


bool file_exists (char *name) {

    std::ifstream f(name);

    if (f.good()) {
        f.close();
        return true;
    } else {
        f.close();
        return false;
    }
}


std::vector<record_t>
get_pool( int argc, char *argv[] )
{
    int i;
    std::vector<record_t> pool;

    if (argc <= 1)
    {
       std::cerr << "Please provide an image as an argument to the program: ./program <image>" << std::endl;
       exit(1);
    }
    else
    {
	    for ( i = 1; i < argc; i++ )
	    {
		if (file_exists(argv[i]))
		        pool.push_back(record_t(argv[i]));
		else
			std::cerr << "File " << argv[i]
				  << " does not exist." << std::endl;
	    }
    }

    return pool;
}
}

#endif
