setlocal

:CheckOpts
if "%~1"=="build" shift & goto Build
if "%~1"=="build-swss-tests" shift & goto BuildSwssTests
if "%~1"=="run-swss-tests" shift & goto RunSwssTests
if "%~1"=="docker-build" shift & goto DockerBuild
if "%~1"=="docker-image-list" shift & goto DockerImageList
if "%~1"=="docker-image-remove" shift & goto DockerImageRemove
if "%~1"=="docker-run-container" shift & goto DockerRunContainer
if "%~1"=="docker-stop-container" shift & goto DockerStopContainer
if "%~1"=="docker-copy-debug-files" shift & goto DockerCopyDebugFiles

:Build
rem ubuntu.exe -c "(cd /mnt/e/build.swss.wsl && make -j8) 2>&1 | sed -e 's/\/mnt\/\(.\)/\U\1:/g'"
docker exec -i -t sonic-swss "(cd /build.swss.wsl && make -j6) 2>&1 | sed -e 's/\/mnt\/\(.\)/\U\1:/g'"
goto :eof

:BuildSwssTests
ubuntu.exe -c "(cd /mnt/e/build.swss.wsl/sonic-swss/tests && make -j8) 2>&1 | sed -e 's/\/mnt\/\(.\)/\U\1:/g'"
goto :eof

:RunSwssTests
ubuntu.exe -c "(cd /mnt/e/build.swss.wsl && source packages/.env && ./sonic-swss/tests/tests --gtest_filter=*.*) 2>&1 | sed -e 's/\/mnt\/\(.\)/\U\1:/g'"
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

:DockerRunContainer
docker-compose up -d
rem ssh root@localhost
rem type C:\Users\yehjunying\.ssh\id_rsa.pub | ssh root@localhost "cat >> .ssh/authorized_keys"
rem docker exec -i -t sonic-swss-dev_dev-image_1 /bin/bash
rem copy ssh from windows folder to this folder

rem find /usr/ -type f -name "*.c" -exec cp {} /mnt/e/Docker/rootfs/ \;
goto :eof

:DockerStopContainer
docker-compose down
goto :eof

:DockerCopyDebugFiles
mkdir -p E:\Docker\rootfs\usr

rem scp -r root@localhost:/usr/include /mnt/e/Docker/rootfs/usr
goto :eof