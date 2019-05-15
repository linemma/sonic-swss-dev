setlocal

:CheckOpts
if "%~1"=="build" shift & goto Build
if "%~1"=="run-tests" shift & goto RunTests
if "%~1"=="build-swss-tests" shift & goto BuildSwssTests
if "%~1"=="run-swss-tests" shift & goto RunSwssTests
if "%~1"=="docker-build" shift & goto DockerBuild
if "%~1"=="docker-image-list" shift & goto DockerImageList
if "%~1"=="docker-image-remove" shift & goto DockerImageRemove
if "%~1"=="docker-compose-up" shift & goto DockerComposeUp
if "%~1"=="docker-compose-down" shift & goto DockerComposeDown
if "%~1"=="docker-copy-debug-files" shift & goto DockerCopyDebugFiles

:BuildWSL
ubuntu.exe -c "(cd /mnt/e/build.swss.wsl && make -j$(nproc)) 2>&1 | sed -e 's/\/mnt\/\(.\)/\U\1:/g'"
goto :eof

:BuildSwssTestsWSL
ubuntu.exe -c "(cd /mnt/e/build.swss.wsl/sonic-swss/tests && make -j8) 2>&1 | sed -e 's/\/mnt\/\(.\)/\U\1:/g'"
goto :eof

:RunSwssTestsWSL
ubuntu.exe -c "(cd /mnt/e/build.swss.wsl && source packages/.env && ./sonic-swss/tests/tests --gtest_filter=*.*) 2>&1 | sed -e 's/\/mnt\/\(.\)/\U\1:/g'"
goto :eof

:Build
for /f "delims=" %%i in ('docker ps -aqf "name=sonic-swss"') do set CONTAINER_ID=%%i
docker exec %CONTAINER_ID% /bin/bash -c "cd /build.swss.wsl && make -j$(nproc)"
goto :eof

:RunTests
for /f "delims=" %%i in ('docker ps -aqf "name=sonic-swss"') do set CONTAINER_ID=%%i
docker exec %CONTAINER_ID% /bin/bash -c "cd /build.swss.wsl && source packages/.env && ./redis/start_redis.sh && ./tests.out"
goto :eof

:BuildSwssTests
for /f "delims=" %%i in ('docker ps -aqf "name=sonic-swss"') do set CONTAINER_ID=%%i
docker exec %CONTAINER_ID% /bin/bash -c "cd /build.swss.wsl && make -j$(nproc) -C ./sonic-swss/tests"
goto :eof

:RunSwssTests
for /f "delims=" %%i in ('docker ps -aqf "name=sonic-swss"') do set CONTAINER_ID=%%i
docker exec %CONTAINER_ID% /bin/bash -c "cd /build.swss.wsl && source packages/.env && ./redis/start_redis.sh && ./sonic-swss/tests/tests"
goto :eof

:DockerBuild
docker build --tag=sonic-swss --force-rm .
goto :eof

:DockerImageList
docker image list -a
goto :eof

:DockerImageRemove
docker image rm sonic-swss
goto :eof

:DockerComposeUp
docker-compose up -d

for /f "delims=" %%i in ('docker ps -aqf "name=sonic-swss"') do set CONTAINER_ID=%%i
docker exec %CONTAINER_ID% /bin/bash -x /sonic-swss-dev/bootstrap.sh
goto :eof

:DockerComposeDown
docker-compose down
goto :eof

:DockerCopyDebugFiles
mkdir -p E:\Docker\rootfs\usr

rem scp -r root@localhost:/usr/include /mnt/e/Docker/rootfs/usr

rem setup ssh
rem ssh root@localhost
rem type C:\Users\yehjunying\.ssh\id_rsa.pub | ssh root@localhost "cat >> .ssh/authorized_keys"

goto :eof