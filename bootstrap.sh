#/bin/bash

apt-get update -y
bash -x /sonic-swss-dev/install-pkg.sh -g
mkdir -p /build.swss.wsl
cd /build.swss.wsl
bash -x /sonic-swss-dev/build.sh
