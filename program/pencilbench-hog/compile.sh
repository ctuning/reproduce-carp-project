KERNEL=$PENCILBENCH_KERNEL
ppcg_tuning_options=$2

HEADER_FLAGS="-I$PENCIL_INCLUDE_DIR/ -I$PRL_INCLUDE_DIR/ -I$CK_ENV_LIB_OPENCL_INCLUDE -I$CK_ENV_LIB_OPENCV_INCLUDE -I$BENCHMARK_ROOT_DIRECTORY/include/"
LINKER_FLAGS="-L$PRL_LIB_DIR/ -L$CK_ENV_LIB_OPENCL_LIB/ -L$CK_ENV_LIB_OPENCV_LIB/"
#LIBRARY_FLAGS="-lprl_opencl -lopencv_core -lopencv_imgproc -lopencv_ocl -lopencv_highgui $CK_ENV_LIB_OPENCL_DYNAMIC_NAME -ltbb -ltbbmalloc"

LIBRARY_FLAGS="-lprl_opencl $CK_PROG_LINKER_LIBS"

#export CK_PROG_LINKER_FLAGS_AFTER=" ${CK_FLAGS_OUTPUT}a.out $CK_LD_FLAGS_MISC $CK_LD_FLAGS_EXTRA  ${CK_FLAG_PREFIX_LIB_DIR}\"/opt/intel/intel-opencl-1.2-4.6.0.92/opencl/lib64\" -lOpenCL  ${CK_FLAG_PREFIX_LIB_DIR}\"/home/fursin/fggwork/ck-tools/lib-opencv-2.4.11-gcc-local-linux-64/build/install/lib\" -lopencv_imgproc  -lopencv_ocl  -lopencv_highgui  -lopencv_core $CK_EXTRA_LIB_M ${CK_CXX_EXTRA} ${CK_ENV_LIB_STDCPP_STATIC}"
#export CK_PROG_COMPILER_FLAGS_BEFORE=" $CK_OPT_SPEED $CK_FLAGS_CREATE_OBJ $CK_COMPILER_FLAGS_OBLIGATORY $CK_FLAGS_DYNAMIC_BIN ${CK_FLAG_PREFIX_INCLUDE}../  $CK_COMPILER_FLAG_CPP11 ${CK_FLAG_PREFIX_INCLUDE}\"/opt/intel/intel-opencl-1.2-4.6.0.92/opencl/include\" ${CK_FLAG_PREFIX_INCLUDE}\"/home/fursin/fggwork/ck-tools/lib-opencv-2.4.11-gcc-local-linux-64/build/install/include\" ${CK_FLAG_PREFIX_INCLUDE}\"/home/fursin/fggwork/ck-tools/pencil-99999-llvm-3.6.0-linux-64/_install/include\" ${CK_FLAG_PREFIX_INCLUDE}\"/home/fursin/fggwork/ck-tools/benchmark-pencil-99999-pencil-99999-linux-64/pencil-benchmark/pencil-benchmarks-imageproc/include\" ${CK_FLAG_PREFIX_INCLUDE}\"../include\" "
#export CK_PROG_COMPILER_FLAGS=" $CK_OPT_SPEED "
#export CK_PROG_LINKER_FLAGS_BEFORE="$CK_COMPILER_FLAGS_OBLIGATORY  $CK_FLAGS_DYNAMIC_BIN"
#export CK_PROG_LINKER_LIBS=" ${CK_FLAG_PREFIX_LIB_DIR}\"/opt/intel/intel-opencl-1.2-4.6.0.92/opencl/lib64\" -lOpenCL  ${CK_FLAG_PREFIX_LIB_DIR}\"/home/fursin/fggwork/ck-tools/lib-opencv-2.4.11-gcc-local-linux-64/build/install/lib\" -lopencv_imgproc  -lopencv_ocl  -lopencv_highgui  -lopencv_core"


echo
echo "[$KERNEL]"

echo "    .ppcg $ppcg_tuning_options"
echo "    .ppcg $PENCIL_COMPILER_EXTRA_OPTIONS $ppcg_tuning_options $HEADER_FLAGS -I$BENCHMARK_ROOT_DIRECTORY/$KERNEL $BENCHMARK_ROOT_DIRECTORY/$KERNEL/$KERNEL.pencil.c" 
$PENCIL_COMPILER_BINARY $PENCIL_COMPILER_EXTRA_OPTIONS $ppcg_tuning_options $HEADER_FLAGS -I$BENCHMARK_ROOT_DIRECTORY/$KERNEL $BENCHMARK_ROOT_DIRECTORY/$KERNEL/$KERNEL.pencil.c 
 if [ "${?}" != "0" ] ; then
  echo "Error: PPCG compilation failed!" 
  exit 1
 fi

echo "    .compiling ${KERNEL}.pencil_host.c and test_${KERNEL}.cpp (g++)"
echo "    .$CK_CXX -x c -c $CK_PROG_COMPILER_FLAGS -DNDEBUG -fomit-frame-pointer -fPIC -std=c99 $HEADER_FLAGS -I$BENCHMARK_ROOT_DIRECTORY/$KERNEL ${KERNEL}.pencil_host.c -o $KERNEL.pencil_host.o" 
$CK_CXX -x c -c $CK_PROG_COMPILER_FLAGS -DNDEBUG -fomit-frame-pointer -fPIC -std=c99 $HEADER_FLAGS -I$BENCHMARK_ROOT_DIRECTORY/$KERNEL ${KERNEL}.pencil_host.c -o $KERNEL.pencil_host.o 
EXIT_STATUS_COMPILATION_1=$?

echo "    .$CK_CXX -shared $CK_PROG_COMPILER_FLAGS -o lib${KERNEL}_ppcg.so $KERNEL.pencil_host.o $LINKER_FLAGS $LIBRARY_FLAGS" 
$CK_CXX -shared  $CK_PROG_COMPILER_FLAGS -o lib${KERNEL}_ppcg.so $KERNEL.pencil_host.o $LINKER_FLAGS $LIBRARY_FLAGS 
EXIT_STATUS_COMPILATION_2=$?

echo "    .$CK_CXX  $CK_PROG_COMPILER_FLAGS $DEFINED_VARIABLES -fomit-frame-pointer -fPIC -std=c++0x $HEADER_FLAGS -Wl,-rpath=RIGIN:$PRL_LIB_DIR $BENCHMARK_ROOT_DIRECTORY/$KERNEL/test_${KERNEL}.cpp -o $CK_PROG_TARGET_EXE $LIBRARY_FLAGS $LINKER_FLAGS -l${KERNEL}_ppcg" 
$CK_CXX  $CK_PROG_COMPILER_FLAGS $DEFINED_VARIABLES -fomit-frame-pointer -fPIC -std=c++0x $HEADER_FLAGS -Wl,-rpath=RIGIN:$PRL_LIB_DIR $BENCHMARK_ROOT_DIRECTORY/$KERNEL/test_${KERNEL}.cpp -o $CK_PROG_TARGET_EXE $LIBRARY_FLAGS $LINKER_FLAGS -l${KERNEL}_ppcg 
EXIT_STATUS_COMPILATION_3=$?

EXIT_STATUS_COMPILATION=`expr $EXIT_STATUS_COMPILATION_1 + $EXIT_STATUS_COMPILATION_2 + $EXIT_STATUS_COMPILATION_3`
 if [ "$EXIT_STATUS_COMPILATION" != "0" ] ; then
  echo "Error: g++ compilation failed!" 
  exit 1
 fi
