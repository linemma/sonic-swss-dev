#/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo $DIR

cd $DIR/../
mkdir acl.build
cd acl.build

echo "git clone swss"
git clone --recursive https://github.com/Azure/sonic-swss.git
echo "git clone sairedis"
git clone --recursive https://github.com/Azure/sonic-sairedis.git

cd sonic-sairedis/SAI/meta
export PERL5LIB=${PWD}
cd ../../
mkdir -p ${PWD}/../install
./autogen.sh
./configure --prefix=$(realpath ${PWD}/../install) --with-sai=vs
make CXXFLAGS="-I/mnt/g/git/acl.build/install/include \
               -Wno-error=long-long \
               -std=c++11 \
               -L/mnt/g/git/acl.build/install/lib"
make install

cd ../

cmake ../acl
