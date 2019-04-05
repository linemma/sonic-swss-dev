#/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

BUILD_PATH=${PWD}
SRC_PATH=$(realpath $(dirname "$0"))
echo "SRC path  ="$SRC_PATH
echo "Build path="${BUILD_PATH}

cd "${SRC_PATH}"
git submodule init
git submodule update --recursive
cd -

#==============================================================================
# Download deps

[ -f "$BUILD_PATH/packages/.env" ] || $DIR/install-pkg.sh

. $BUILD_PATH/packages/.env

cd $SRC_PATH
mkdir -p ${BUILD_PATH}/install/include/swss

#==============================================================================
# Build swsscommon

build_swsscommon()
{
    echo Building swsscommon ...

    SWSS_COMMON_PATH="${SRC_PATH}/sonic-swss-common"

    sed -i '/CFLAGS_COMMON+=" -Werror"/d' ${SWSS_COMMON_PATH}/configure.ac
    sed -i 's|_swsscommon_la_CPPFLAGS = -std=c++11 -I../common|_swsscommon_la_CPPFLAGS = -std=c++11 -I$(top_srcdir)/common|g' ${SWSS_COMMON_PATH}/pyext/Makefile.am
    sed -i 's|_swsscommon_la_LIBADD = ../common/libswsscommon.la|_swsscommon_la_LIBADD = $(top_builddir)/common/libswsscommon.la|g' ${SWSS_COMMON_PATH}/pyext/Makefile.am
    sed -i 's|$(SWIG) -c++ -python -I../common|$(SWIG) -c++ -python -I$(top_srcdir)/common|g' ${SWSS_COMMON_PATH}/pyext/Makefile.am
    sed -i 's|-L$(top_srcdir)/common|-L$(top_builddir)/common|g' ${SWSS_COMMON_PATH}/tests/Makefile.am

    if [ ! -f "${SRC_PATH}/sonic-swss-common/configure" ]; then
        cd ${SWSS_COMMON_PATH}
        ./autogen.sh
        make distclean
    fi

    cd "${SRC_PATH}/sonic-swss-common"
    make distclean

    mkdir -p "${BUILD_PATH}/sonic-swss-common"
    cd "${BUILD_PATH}/sonic-swss-common"
    "${SRC_PATH}/sonic-swss-common/configure" --prefix=$(realpath ${BUILD_PATH}/install )
    make -j 3 && make install

    if [ "$?" -ne "0" ]; then
        echo "Failed to build swss-common"
        exit 1
    fi

    rm -rf ${SWSS_COMMON_PATH}/m4
    rm -rf ${SWSS_COMMON_PATH}/autom4te.cache
    rm -rf ${SWSS_COMMON_PATH}/config.h.in~

    cd "${SRC_PATH}/sonic-swss-common"
    cp ./common/*.h ${BUILD_PATH}/install/include/swss
    cp ./common/*.hpp ${BUILD_PATH}/install/include/swss
}

[[ -f ${BUILD_PATH}/install/lib/libswsscommon.a && -f ${BUILD_PATH}/install/lib/libswsscommon.so ]] || build_swsscommon

#==============================================================================
# Build sairedis

# : <<'BUILD-SAIREDIS'
echo Building sairedis ...

SAIREDIS_PATH="${SRC_PATH}/sonic-sairedis"

sed -i '/CFLAGS_COMMON+=" -Werror"/d' ${SAIREDIS_PATH}/configure.ac
sed -i '/-Werror \\/d' ${SAIREDIS_PATH}/meta/Makefile.am

cd ${SAIREDIS_PATH}/SAI/meta
export PERL5LIB=${PWD}
make saimetadata.c saimetadata.h
if [ "$?" -ne "0" ]; then
    echo "Failed to build saimetadata"
    exit 1
fi
cd ${SAIREDIS_PATH}
./autogen.sh
./configure --prefix=$(realpath ${BUILD_PATH}/install) --with-sai=vs
make -j 3 CXXFLAGS="-I$(realpath ${BUILD_PATH}/install/include) \
            -Wno-error=long-long \
            -std=c++11 \
            -L$(realpath ${BUILD_PATH}/install/lib) $CXXFLAGS"
if [ "$?" -ne "0" ]; then
    echo "Failed to build sairedis"
    exit 1
fi
make install

# BUILD-SAIREDIS

#==============================================================================
# Build swss

# : <<'BUILD-SWSS'
echo Build SWSS ...

# TODO: using /usr/bin/patch instead of

sed -i 's|CFLAGS_COMMON+=" -Werror"|#CFLAGS_COMMON+=" -Werror"|g' ${SRC_PATH}/sonic-swss/configure.ac

sed -i 's|string str = counterIdsToStr(c_portStatIds, &sai_serialize_port_stat);|string str = counterIdsToStr(c_portStatIds, static_cast<string (*)(const sai_port_stat_t)>(\&sai_serialize_port_stat));|g' ${SRC_PATH}/sonic-swss/orchagent/pfcwdorch.cpp
sed -i 's|string str = counterIdsToStr(c_queueStatIds, sai_serialize_queue_stat);|string str = counterIdsToStr(c_queueStatIds, static_cast<string (*)(const sai_queue_stat_t)>(\&sai_serialize_queue_stat));|g' ${SRC_PATH}/sonic-swss/orchagent/pfcwdorch.cpp

cd ${BUILD_PATH}
cmake ${SRC_PATH} -DCMAKE_CXX_FLAGS="$CXXFLAGS $LIBS" -DGTEST_ROOT_DIR=$(pkg-config --variable=prefix googletest)
make -j 3

# BUILD-SWSS
