#/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
echo $DIR

cd $DIR/../
mkdir acl.build
cd acl.build

echo "git clone swss"
git clone https://github.com/Azure/sonic-swss.git
echo "git clone sairedis"
git clone https://github.com/Azure/sonic-sairedis.git

cd sonic-sairedis
git submodule init
git submodule update
cd ../

cmake ../acl
