#/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

BUILD_PATH=${PWD}
SRC_PATH=$(realpath $(dirname "$0"))
echo "SRC path  ="$SRC_PATH
echo "Build path="${BUILD_PATH}

print_help() {
    echo "hello"
}

download_source_code() {
    cd "${SRC_PATH}"
    git submodule init
    git submodule update --recursive
    cd -
}

download_deps() {
    [ -f "$BUILD_PATH/packages/.env" ] || $DIR/install-pkg.sh

    . $BUILD_PATH/packages/.env
}

build_swsscommon() {
    echo "Building sonic-swss-common ..."

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
    make "-j$(nproc)"

    if [ "$?" -ne "0" ]; then
        echo "Failed to build swss-common"
        exit 1
    fi

    make install

    rm -rf ${SWSS_COMMON_PATH}/m4
    rm -rf ${SWSS_COMMON_PATH}/autom4te.cache
    rm -rf ${SWSS_COMMON_PATH}/config.h.in~

    cd "${SRC_PATH}/sonic-swss-common"
    cp ./common/*.h ${BUILD_PATH}/install/include/swss
    cp ./common/*.hpp ${BUILD_PATH}/install/include/swss
}

build_sairedis() {
    echo "Building sonic-sairedis ..."

    SAIREDIS_PATH="${SRC_PATH}/sonic-sairedis"

    # sed -i '/-Werror \\/d' ${SAIREDIS_PATH}/meta/Makefile.am
    sed -i '/-Wmissing-include-dirs \\/d' ${SAIREDIS_PATH}/meta/Makefile.am

    # cd ${SAIREDIS_PATH}/SAI/meta
    # export PERL5LIB=${PWD}
    # make saimetadata.c saimetadata.h
    # if [ "$?" -ne "0" ]; then
    #     echo "Failed to build saimetadata"
    #     exit 1
    # fi

    if [ ! -e "${BUILD_PATH}/sonic-sairedis/Makefile" ]; then
        cd "${SRC_PATH}/sonic-sairedis"
        ./autogen.sh
        sed -i '/CFLAGS_COMMON+=" -Werror"/d' ${SAIREDIS_PATH}/configure.ac

        mkdir -p "${BUILD_PATH}/sonic-sairedis"
        cd "${BUILD_PATH}/sonic-sairedis"

        #                                                                                     for #include "meta/sai_meta.h"
        "${SAIREDIS_PATH}/configure" --prefix=$(realpath ${BUILD_PATH}/install) --with-sai=vs CXXFLAGS="-I${SAIREDIS_PATH} -I$(realpath ${BUILD_PATH}/install/include) \
        -Wno-error=long-long \
        -std=c++11 \
        -L$(realpath ${BUILD_PATH}/install/lib) $CXXFLAGS"
    fi

    echo "Generating saimetadata.c and saimetadata.h ..."

    # mkdir -p "${BUILD_PATH}/sonic-sairedis/SAI/meta"
    # cd "${BUILD_PATH}/sonic-sairedis/SAI/meta"
    cd ${SAIREDIS_PATH}/SAI/meta
    export PERL5LIB=${PWD}
    # export PERL5LIB="${SAIREDIS_PATH}/SAI/meta"
    # make -C ${SAIREDIS_PATH}/SAI/meta saimetadata.c saimetadata.h
    make saimetadata.c saimetadata.h
    if [ "$?" -ne "0" ]; then
        echo "Failed to build saimetadata"
        exit 1
    fi

    echo "Build sairedis ..."

    mkdir -p "${BUILD_PATH}/sonic-sairedis"
    cd "${BUILD_PATH}/sonic-sairedis"

    # make -C "${BUILD_PATH}/sonic-sairedis/meta" && make "-j$(nproc)"
    make "-j$(nproc)"
    if [ "$?" -ne "0" ]; then
        echo "Failed to build sairedis"
        exit 1
    fi
    make install
}

build_swss_orchagent() {
    echo "Build sonic-swss-orchagent ..."

    # TODO: using /usr/bin/patch instead of

    sed -i 's|CFLAGS_COMMON+=" -Werror"|#CFLAGS_COMMON+=" -Werror"|g' ${SRC_PATH}/sonic-swss/configure.ac

    sed -i 's|string str = counterIdsToStr(c_portStatIds, &sai_serialize_port_stat);|string str = counterIdsToStr(c_portStatIds, static_cast<string (*)(const sai_port_stat_t)>(\&sai_serialize_port_stat));|g' ${SRC_PATH}/sonic-swss/orchagent/pfcwdorch.cpp
    sed -i 's|string str = counterIdsToStr(c_queueStatIds, sai_serialize_queue_stat);|string str = counterIdsToStr(c_queueStatIds, static_cast<string (*)(const sai_queue_stat_t)>(\&sai_serialize_queue_stat));|g' ${SRC_PATH}/sonic-swss/orchagent/pfcwdorch.cpp

    cd ${BUILD_PATH}
    cmake ${SRC_PATH} -DCMAKE_CXX_FLAGS="$CXXFLAGS $LIBS" -DGTEST_ROOT_DIR=$(pkg-config --variable=prefix googletest)
    make "-j$(nproc)"
}

build_all () {
    [[ -f ${BUILD_PATH}/install/lib/libswsscommon.a && -f ${BUILD_PATH}/install/lib/libswsscommon.so ]] || build_swsscommon
    build_sairedis
    build_swss_orchagent
}

main() {
    while [[ $# -ne 0 ]]
    do
        arg="$1"
        case "$arg" in
            -h|--help)
                print_help
                exit 0
                ;;
            -c|--clean)
                clean_all
                exit 0
                ;;
            *)
                echo >&2 "Invalid option \"$arg\""
                print_help
                exit 1
        esac
        shift
    done

    download_source_code
    download_deps

    cd $SRC_PATH
    mkdir -p ${BUILD_PATH}/install/include/swss

    build_all
}

main "$@"
