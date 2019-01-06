# Is netcdf-4 and/or DAP enabled?
NC4=1
DAP=1
#CDF5=1
#HDF4=1
S3=1

for arg in "$@" ; do
case "$arg" in
nobuild|nb) NOBUILD=1 ;;
notest|nt) NOTEST=1 ;;
*) echo "unknown option $arg"; exit 1; ;;
esac
done

#TESTSERVERS="localhost:8080,149.165.169.123:8080"
#HDF5VERSION=1.10.4

#export NCPATHDEBUG=1

NCLIB=`pwd`
NCLIB="${NCLIB}/build/liblib"
PATH="/cygdrive/c/tools/CMake/bin:$PATH"
PATH="/cygdrive/c/tools/nccmake/bin:$PATH"
if test -e /cygdrive/c/tools/libaec ; then
PATH="$PATH:/cygdrive/c/tools/libaec:$PATH"
export PATH="${NCLIB}:$PATH"
fi

FLAGS=	
#FLAGS="${FLAGS} -DCMAKE_PREFIX_PATH=c:/tools/nccmake;c:/tools/nccmake/cmake"
FLAGS="${FLAGS} -DCMAKE_PREFIX_PATH=c:/tools/nccmake/cmake"
FLAGS="$FLAGS -DCMAKE_INSTALL_PREFIX=c:/temp/netcdf"

if test "x$DAP" = x ; then
FLAGS="$FLAGS -DENABLE_DAP=false"
fi
if test "x$NC4" = x ; then
FLAGS="$FLAGS -DENABLE_NETCDF_4=false"
fi
if test "x$CDF5" != x ; then
FLAGS="$FLAGS -DENABLE_CDF5=true"
fi
if test "x$HDF4" != x ; then
FLAGS="$FLAGS -DENABLE_HDF4=true"
fi
if test "x$S3" != "x" ; then
  FLAGS="$FLAGS -DENABLE_BYTERANGE=true"
fi

if test "x$TESTSERVERS" != x ; then
FLAGS="$FLAGS -DREMOTETESTSERVERS=${TESTSERVERS}"
fi

# Enables
FLAGS="$FLAGS -DENABLE_DAP_REMOTE_TESTS=true"
FLAGS="$FLAGS -DENABLE_LOGGING=true"
#FLAGS="$FLAGS -DENABLE_DOXYGEN=true -DENABLE_INTERNAL_DOCS=true"
#FLAGS="$FLAGS -DENABLE_LARGE_FILE_TESTS=true"
FLAGS="$FLAGS -DENABLE_FILTER_TESTING=false"

# Disables
FLAGS="$FLAGS -DENABLE_EXAMPLES=false"
FLAGS="$FLAGS -DENABLE_CONVERSION_WARNINGS=false"
#FLAGS="$FLAGS -DENABLE_TESTS=false"
#FLAGS="$FLAGS -DENABLE_DISKLESS=false"

# Withs
FLAGS="$FLAGS -DNCPROPERTIES_EXTRA=\"key1=value1|key2=value2\""

if test "x$HDF5VERSION" != x ; then
FLAGS="$FLAGS -DHDF5_VERSION=$HDF5VERSION"
fi

rm -fr build
mkdir build
cd build

NCLIB=`pwd`

CFG="Release"
NCLIB="${NCLIB}/liblib"
export PATH="${NCLIB}:${PATH}"
#G="G=Visual Studio 15 2017 Win64"
#V="-DCMAKE_VERBOSE_MAKEFILE=on"
#T="--trace-expand"
cmake "$G" $V -DCMAKE_BUILD_TYPE=${CFG} $T $FLAGS ..
if test "x$NOBUILD" = x ; then
cmake --build . --config ${CFG}
if test "x$NOTEST" = x ; then
cmake --build . --config ${CFG} --target RUN_TESTS
fi
fi
exit
