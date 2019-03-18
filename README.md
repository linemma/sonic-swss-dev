# Redis open UNIX socket
// https://www.mf8.biz/apt-get-install-redis-server/
```
apt-get install redis-server
apt update
apt install redis-server
sudo ps aux | grep redis   >> check redis run on which user
sudo usermod -g www-data redis
sudo mkdir -p /var/run/redis/
sudo chown -R redis:www-data /var/run/redis
sudo vim /etc/redis/redis.conf
  // Find following string and remove "#"
    unixsocket /var/run/redis/redis.sock
    unixsocketperm 777
service redis-server restart
```

# Install tools
## swss-common
```
sudo apt-get install make libtool m4 autoconf dh-exec debhelper cmake pkg-config \
                     libhiredis-dev libnl-3-dev libnl-genl-3-dev libnl-route-3-dev swig3.0 \
                     libpython2.7-dev libgtest-dev

cd /usr/src/gtest && sudo cmake . && sudo make
```

## SAI
```
sudo apt install -y doxygen
sudo apt install -y graphviz
sudo apt install -y aspell
```

## sonic-swss
```
sudo apt-get install -y libhiredis0.13 -t trusty
```


# Build the test
```
cd acl/
./build.sh
cd ../acl.build
make
```
