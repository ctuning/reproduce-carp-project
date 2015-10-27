# Added by Grigori Fursin to be able to compile, run and autotune
# this benchmark via CK

KERNEL=$PENCILBENCH_KERNEL

# Assemble PPCG tuning options from other variables
#  which can be externaly tuned via CK

ppcg_tuning_options=""

if [ "$PPCG_FLAG_LOOP_FUSION" != "" ] ; then
  ppcg_tuning_options="${ppcg_tuning_options} $PPCG_FLAG_LOOP_FUSION"
fi

if [ "$PPCG_FLAG_SHARED_MEM" != "" ] ; then
  ppcg_tuning_options="${ppcg_tuning_options} $PPCG_FLAG_SHARED_MEM"
fi

if [ "$PPCG_FLAG_PRIVATE_MEM" != "" ] ; then
  ppcg_tuning_options="${ppcg_tuning_options} $PPCG_FLAG_PRIVATE_MEM"
fi

ppcg_flag_sizes=""

if [ "$PPCG_FLAG_TILE" != "" ] ; then
   ppcg_flag_sizes="kernel[i]->tile[$PPCG_FLAG_TILE]"
fi

if [ "$PPCG_FLAG_GRID" != "" ] ; then
   if [ "$ppcg_flag_sizes" != "" ] ; then
      ppcg_flag_sizes="$ppcg_flag_sizes;"
   fi
   ppcg_flag_sizes="${ppcg_flag_sizes}kernel[i]->grid[$PPCG_FLAG_GRID]"
fi

if [ "$PPCG_FLAG_BLOCK" != "" ] ; then
   if [ "$ppcg_flag_sizes" != "" ] ; then
      ppcg_flag_sizes="$ppcg_flag_sizes;"
   fi
   ppcg_flag_sizes="${ppcg_flag_sizes}kernel[i]->block[$PPCG_FLAG_BLOCK]"
fi

if [ "$ppcg_flag_sizes" != "" ] ; then
  ppcg_tuning_options="${ppcg_tuning_options} --sizes={$ppcg_flag_sizes}"
fi

# Continue assembling various vars for compilation and linking
HEADER_FLAGS="-I$PENCIL_INCLUDE_DIR/ -I$PRL_INCLUDE_DIR/ -I$CK_ENV_LIB_OPENCL_INCLUDE -I$CK_ENV_LIB_OPENCV_INCLUDE -I$BENCHMARK_ROOT_DIRECTORY/include/"
LINKER_FLAGS="-L$PRL_LIB_DIR/ -L$CK_ENV_LIB_OPENCL_LIB/ -L$CK_ENV_LIB_OPENCV_LIB/"
LIBRARY_FLAGS="-lprl_opencl $CK_PROG_LINKER_LIBS"

echo
echo "[$KERNEL]"

echo "    .ppcg $ppcg_tuning_options"
echo "    .ppcg $PENCIL_COMPILER_EXTRA_OPTIONS --target=prl $ppcg_tuning_options $HEADER_FLAGS -I$BENCHMARK_ROOT_DIRECTORY/$KERNEL $BENCHMARK_ROOT_DIRECTORY/$KERNEL/$KERNEL.pencil.c" 
$PENCIL_COMPILER_BINARY $PENCIL_COMPILER_EXTRA_OPTIONS --target=prl $ppcg_tuning_options $HEADER_FLAGS -I$BENCHMARK_ROOT_DIRECTORY/$KERNEL $BENCHMARK_ROOT_DIRECTORY/$KERNEL/$KERNEL.pencil.c 
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
