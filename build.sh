#/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

BUILD_PATH=${PWD}
SRC_PATH=$(realpath $(dirname "$0"))
echo "SRC path  ="$SRC_PATH
echo "Build path="${BUILD_PATH}

[ -f "$BUILD_PATH/packages/.env" ] || $DIR/install-pkg.sh

. $BUILD_PATH/packages/.env

cd $SRC_PATH
mkdir -p ${BUILD_PATH}/install/include/swss

# cd sonic-swss/
# git apply ../patch/swss_pfcwdorch.diff
# cd ../

# : <<'BUILD-SWSSCOMMON'

# Build sonic-swss-common
SWSS_COMMON_PATH="${SRC_PATH}/sonic-swss-common"

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

cd "${SRC_PATH}/sonic-swss-common"
cp ./common/*.h ${BUILD_PATH}/install/include/swss
cp ./common/*.hpp ${BUILD_PATH}/install/include/swss

# BUILD-SWSSCOMMON

SAIREDIS_PATH="${SRC_PATH}/sonic-sairedis"
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

# TODO: using /usr/bin/patch instead of

sed -i 's|CFLAGS_COMMON+=" -Werror"|#CFLAGS_COMMON+=" -Werror"|g' ${SRC_PATH}/sonic-swss/configure.ac

sed -i 's|string str = counterIdsToStr(c_portStatIds, &sai_serialize_port_stat);|string str = counterIdsToStr(c_portStatIds, static_cast<string (*)(const sai_port_stat_t)>(\&sai_serialize_port_stat));|g' ${SRC_PATH}/sonic-swss/orchagent/pfcwdorch.cpp

sed -i 's|string str = counterIdsToStr(c_queueStatIds, sai_serialize_queue_stat);|string str = counterIdsToStr(c_queueStatIds, static_cast<string (*)(const sai_queue_stat_t)>(\&sai_serialize_queue_stat));|g' ${SRC_PATH}/sonic-swss/orchagent/pfcwdorch.cpp

cd ${BUILD_PATH}
cmake ${SRC_PATH} -DGTEST_ROOT_DIR=$(pkg-config --variable=prefix googletest)

