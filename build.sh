#/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
SRC_PATH=${DIR}
echo "SRC path  ="$SRC_PATH

cd $SRC_PATH
BUILD_DIR_NAME="${PWD##*/}.build"
BUILD_PATH=$(realpath ${SRC_PATH}/../${BUILD_DIR_NAME})
echo "Build path="${BUILD_PATH}

mkdir -p ${BUILD_PATH}/install/include/swss
#cd ${BUILD_DIR}

#echo "git clone swss"
#git clone --recursive https://github.com/Azure/sonic-swss.git
#echo "git clone sairedis"
#git clone --recursive https://github.com/Azure/sonic-sairedis.git

# Build sonic-swss-common
SWSS_COMMON_PATH="${SRC_PATH}/sonic-swss-common"
cd ${SWSS_COMMON_PATH}
./autogen.sh
./configure --prefix=$(realpath ${BUILD_PATH}/install )
make && make install
if [ "$?" -ne "0" ]; then
  echo "Failed to build swss-common"
  exit 1
fi
cp ./common/*.h ${BUILD_PATH}/install/include/swss
cp ./common/*.hpp ${BUILD_PATH}/install/include/swss

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
make CXXFLAGS="-I$(realpath ${BUILD_PATH}/install/include) \
               -Wno-error=long-long \
               -std=c++11 \
               -L$(realpath ${BUILD_PATH}/install/lib)"
if [ "$?" -ne "0" ]; then
  echo "Failed to build sairedis"
  exit 1
fi
make install

cd ${BUILD_PATH}
cmake ${SRC_PATH}

