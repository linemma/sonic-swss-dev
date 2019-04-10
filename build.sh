#/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

BUILD_PATH=${PWD}
SRC_PATH=$(realpath $(dirname "$0"))
echo "SRC path  ="$SRC_PATH
echo "Build path="${BUILD_PATH}

print_help() {
    echo "hello"
}

apply_patch() {
    local files, i

    sed -i '/CFLAGS_COMMON+=" -Werror"/d' "${SRC_PATH}/sonic-sairedis/configure.ac"
    sed -i '/-Wmissing-include-dirs \\/d' "${SRC_PATH}/sonic-sairedis/meta/Makefile.am"

    if ! grep -q tests_DEPENDENCIES "${SRC_PATH}/sonic-sairedis/meta/Makefile.am"; then
        sed -i '/tests_LDADD = -lhiredis -lswsscommon/atests_DEPENDENCIES = libsaimeta.la libsaimetadata.la' "${SRC_PATH}/sonic-sairedis/meta/Makefile.am"
    fi

    if ! grep -q tests_DEPENDENCIES "${SRC_PATH}/sonic-sairedis/vslib/src/Makefile.am"; then
        sed -i '/tests_LDADD = -lhiredis -lswsscommon/atests_DEPENDENCIES = libsaivs.la' "${SRC_PATH}/sonic-sairedis/vslib/src/Makefile.am"
    fi

    files=("${SRC_PATH}/sonic-sairedis/meta/Makefile.am" "${SRC_PATH}/sonic-sairedis/vslib/src/Makefile.am")
    files+=("${SRC_PATH}/sonic-sairedis/saidiscovery/Makefile.am")
    files+=("${SRC_PATH}/sonic-sairedis/saidump/Makefile.am")
    files+=("${SRC_PATH}/sonic-sairedis/saiplayer/Makefile.am")
    files+=("${SRC_PATH}/sonic-sairedis/saisdkdump/Makefile.am")
    files+=("${SRC_PATH}/sonic-sairedis/syncd/Makefile.am")
    files+=("${SRC_PATH}/sonic-sairedis/tests/Makefile.am")
    for i in "${files[@]}"
    do
	    sed -i 's|-L$(top_srcdir)|-L$(top_builddir)|g' "$i"
    done
}

download_source_code() {
    cd "${SRC_PATH}"
    git submodule init
    git submodule update --recursive

    apply_patch
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

    if [ ! -e "${BUILD_PATH}/sonic-sairedis/Makefile" ]; then
        cd "${SRC_PATH}/sonic-sairedis"
        ./autogen.sh

        mkdir -p "${BUILD_PATH}/sonic-sairedis"
        cd "${BUILD_PATH}/sonic-sairedis"

        #                                                                                     for #include "meta/sai_meta.h"
        "${SAIREDIS_PATH}/configure" --prefix=$(realpath ${BUILD_PATH}/install) --with-sai=vs CXXFLAGS="-I${SAIREDIS_PATH} -I$(realpath ${BUILD_PATH}/install/include) \
        -Wno-error=long-long \
        -std=c++11 \
        -L$(realpath ${BUILD_PATH}/install/lib) $CXXFLAGS"
    fi

    echo "Generating saimetadata.c and saimetadata.h ..."

    cd ${SAIREDIS_PATH}/SAI/meta
    export PERL5LIB=${PWD}
    make saimetadata.c saimetadata.h
    if [ "$?" -ne "0" ]; then
        echo "Failed to build saimetadata"
        exit 1
    fi

    echo "Build sairedis ..."

    mkdir -p "${BUILD_PATH}/sonic-sairedis"
    cd "${BUILD_PATH}/sonic-sairedis"

    make "-j$(nproc)"
    if [ "$?" -ne "0" ]; then
        echo "Failed to build sairedis"
        exit 1
    fi
    make install
}

build_swss_orchagent() {
    echo "Build sonic-swss-orchagent ..."

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
