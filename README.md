# Introduction
ACL unit test environment for SONiC

# Getting Started
## Install requirment tools
```
# sswss-common
sudo apt-get install make libtool m4 autoconf dh-exec debhelper cmake pkg-config \
                     libhiredis-dev libnl-3-dev libnl-genl-3-dev libnl-route-3-dev swig3.0 \
                     libpython2.7-dev libgtest-dev

cd /usr/src/gtest && sudo cmake . && sudo make

# SAI
sudo apt install -y doxygen
sudo apt install -y graphviz
sudo apt install -y aspell

# sonic-swss
sudo apt-get install -y libhiredis0.13 -t trusty
```

### install perl  module
```sudo perl -MCPAN -e shell```  
>```cpan[1]> install XML::Simple ```  
>``` ... ```  
>```cpan[2]> exit```  

## Build the test environment
```
git clone --recurse-submodules --shallow-submodules -j4 https://github.com/ezio-chen/sonic-swss-acl-dev.git
cd sonic-swss-acl-dev
./build.sh
cd ../sonic-swss-acl-dev.build
make
```

# Starting redis-server and open UNIX socket
```
sudo apt update
sudo apt install redis-server
sudo service redis-server start
sudo ps aux | grep redis                     -> check redis-server run on which user
sudo usermod -g www-data redis               -> change user(redis) to GID(ww-data)
sudo mkdir -p /var/run/redis/
sudo chown -R redis:www-data /var/run/redis
sudo vim /etc/redis/redis.conf
  // Find following string and remove "#"
    unixsocket /var/run/redis/redis.sock
    unixsocketperm 777
sudo service redis-server restart
```

