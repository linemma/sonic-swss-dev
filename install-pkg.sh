_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

_pkg_scripts_home=${1:-$_DIR}
_package_cfg_path=${2:-"$_DIR"/package.cfg}
_build_home=${3:-$(pwd)}

_script_home=$_pkg_scripts_home

. $_DIR/package-installer/cli.sh "$_pkg_scripts_home" "$_package_cfg_path" "$_build_home"

# exit 0

# script_home=${1:-$(dirname "$0")}
# build_home=${2:-$(pwd)}

# pkgs_dir="$script_home"

# #SRC_PATH=$(realpath $(dirname "$0"))

# install_prefix="$build_home/packages" # this is default path for prefix. Let user can pass prefix from command line
# packages=()
# cflags=()
# libs=()
# ld_library_path=()
# path=()

# if [ ! -d "$install_prefix" ]; then
#     mkdir $install_prefix
# fi

# function change_link
# {
#     local dir="$1"
#     local _f

#     for _f in $(find $dir -type l); do
#         target="$(readlink $_f)"
#         if [ ! -f "$target" ]; then
#             if $(find "$dir" -name $(basename -- "$target") 1> /dev/null 2>&1); then
#                 echo FOUND
#                 rm "$_f"
#                 ln -s "$(find "$dir" -name $(basename -- "$target"))" "$_f"
#             else
#                 echo NOT FOUND
#             fi
#         fi
#     done
# }

# # change_link "$install_prefix"

# function join_by { local IFS="$1"; shift; echo "$*"; }

# function contains_element
# {
#     local e match="$1"
#     shift
#     for e; do [[ "$e" == "$match" ]] && return 0; done
#     return 1
# }

# # array=("something to search for" "a string" "test2000")
# # if contains_element "a string" "${array[@]}"; then
# #     echo OK
# # else
# #     echo ERROR
# # fi
# # 
# # if ! contains_element "blaha" "${array[@]}"; then
# #     echo OK
# # else
# #     echo ERROR
# # fi

# function add_to_pkg_config_path
# {
#     local dir re

#     for dir; do
#         re="(^$dir:|:$dir:|:$dir$)"
#         if ! [[ $PKG_CONFIG_PATH =~ $re ]]; then

#             if [[ -z "$PKG_CONFIG_PATH" ]]; then
#                 export PKG_CONFIG_PATH=$dir
#             else
#                 PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$dir
#             fi

#         fi
#     done
# }

# # $1 package name
# function is_avail_package
# {
#     return $(apt-cache show $1 1> /dev/null 2>&1)
# }

# # if is_avail_package libc6; then
# #     echo found libc6
# # else
# #     echo ERROR
# # fi
# # 
# # if is_avail_package xx; then
# #     echo ERROR
# # else
# #     echo not found xx
# # fi

# # $1 package name
# function is_package_installed
# {
#     local pkg="$1"

#     if contains_element "$pkg" "${packages[@]}"; then
#         return 0    # installed
#     fi

#     if [ "" == "$(dpkg-query -W --showformat='${Status}\n' $pkg | grep 'install ok installed')" ]; then
#         return 1    # not installed
#     fi

#     return 0        # installed
# }

# # if is_package_installed libc6; then
# #     echo libc6 install
# # else
# #     echo ERROR
# # fi
# # 
# # if is_package_installed libnl-3; then
# #     echo ERROR
# # else
# #     echo libnl-3 was not installed
# # fi

# # $1 deb filepath
# function query_package_depends
# {
#     local deb_path=$1
#     local pkg_deps=()

#     # depends=' Depends: libc6 (>= 2.14), libclang1-6.0 (>= 1:5.0~svn298832-1~), libgcc1 (>= 1:3.0), libstdc++6 (>= 5.2), libxapian30'
#     # depends=' Depends: ucf (>= 0.29), fonts-dejavu-core | ttf-bitstream-vera | fonts-liberation | fonts-freefont
#     # local depends=$(dpkg -I $(ls $pkg*.deb) | grep Depends:)
#     local depends=$(dpkg -I $deb_path | grep Depends:)
#     depends=${depends/Depends:/}
#     depends=${depends//[[:blank:]]/}

#     OIFS="$IFS"
#     IFS=',|'
#     read -ra depends <<< "${depends}"
#     IFS="$OIFS"

#     for dep_pkg in "${depends[@]}"; do
#         if [[ $dep_pkg =~ ([^\(]*) ]]; then

#             local pkg_name="${BASH_REMATCH[1]}"
             
#             # local PKG_OK=$(dpkg-query -W --showformat='${Status}\n' $pkg_name | grep "install ok installed")
#             # 
#             # if [ "" == "$PKG_OK" ]; then
#             #     pkg_deps+=($pkg_name)
#             # fi
#             if ! is_package_installed $pkg_name; then
#                 pkg_deps+=($pkg_name)
#             fi
#         fi
#     done

#     echo ${pkg_deps[@]}
# }

# #
# # test by download deb file and get depends from it ....
# #
# # _pkg_deps=($(query_package_depends /mnt/e/packages/libhiredis-dev/libhiredis-dev_0.13.3-2.2_amd64.deb))
# # for pkg in ${_pkg_deps[@]}; do
# #     echo $pkg
# # done
# # 
# # _pkg_deps=($(query_package_depends /mnt/e/doxygen_1.8.13-10_amd64.deb))
# # for pkg in ${_pkg_deps[@]}; do
# #     echo $pkg
# # done
# # 
# # _pkg_deps=($(query_package_depends /mnt/e/fontconfig-config_2.12.6-0ubuntu2_all.deb))
# # for pkg in ${_pkg_deps[@]}; do
# #     echo $pkg
# # done

# # $1 package name
# # $2 tmp folder for downloading deb file
# function get_deb_filepath
# {
#     local pkg=$1
#     local tmpdir="${2:-$(pwd)}"

#     if ! ls $tmpdir/$pkg*.deb 1> /dev/null 2>&1; then
#         return 0
#     else
#         echo $(ls $tmpdir/$pkg*.deb)
#     fi
# }

# # deb_path=$(get_deb_filepath doxygen)
# # echo $deb_path
# # 
# # no_path=$(get_deb_filepath xxx)
# # echo $no_path

# # $1 package name
# # $2 tmp folder for downloading deb file
# function download_package
# {
#     local pkg=$1
#     local tmpdir="${2:-$(pwd)}"

#     if ! is_avail_package $1; then
#         return 0
#     fi

#     [ -d "${tmpdir}" ] || mkdir -p "${tmpdir}"

#     cd "${tmpdir}"

#     if ! ls $pkg*.deb 1> /dev/null 2>&1; then
#         apt-get download $pkg
#     fi

#     cd -
# }

# # if ! download_package noavailpackage; then
# #     echo not avail package
# # fi
# # 
# # if download_package doxygen; then
# #     echo download package doxygen
# # fi

# # $1 package name
# # $2 install folder
# # $3 download deb folder
# function install_one_package
# {
#     local pkg=$1
#     local install_pkg_prefix=$2/${pkg}
#     local deb_tmp_dir=$3/${pkg}

#     if ! is_avail_package $1; then
#         return 0
#     fi

#     download_package $pkg "$deb_tmp_dir"

#     local deb_path=$(get_deb_filepath $pkg "$deb_tmp_dir")
#     local _pkg_deps=($(query_package_depends $deb_path))

#     for _pkg in ${_pkg_deps[@]}; do
#         install_one_package $_pkg "$2" "$3"
#     done

#     if is_package_installed "$pkg"; then
#         return 0
#     fi

#     dpkg -x `ls $deb_tmp_dir/$pkg*.deb` $install_pkg_prefix

#     if [ -f "$script_home/$pkg-post-deb-extract" ]; then
#         $script_home/$pkg-post-deb-extract "$install_pkg_prefix"
#     fi

#     change_link "$2"

#     for _pkg_path in $(find $install_pkg_prefix -name *.pc); do   
#         if [ -f "$_pkg_path" ]; then
#             pkg_dir=$(dirname "$_pkg_path")
#             pkg_name=$(basename -- "$_pkg_path")
#             pkg_name="${pkg_name%.*}"

#             add_to_pkg_config_path "$pkg_dir"
            
#             old_prefix=$(pkg-config --variable=prefix $pkg_name)
#             pkg-config --define-variable=prefix="$install_pkg_prefix$old_prefix" --cflags $pkg_name
#             # packages+=($pkg_name)
#             cflags+=($(pkg-config --define-variable=prefix="$install_pkg_prefix$old_prefix" --cflags $pkg_name))
#             echo clags="${cflags[@]}"
#             libs+=($(pkg-config --define-variable=prefix="$install_pkg_prefix$old_prefix" --libs $pkg_name))
#         fi
#     done

#     if [ -d "$install_pkg_prefix/usr/bin" ]; then
#         path+=("$install_pkg_prefix/usr/bin")
#         PATH="$install_pkg_prefix/usr/bin":$PATH
#     fi

#     for _so_path in $(find $install_pkg_prefix -name *.so*); do
#         if [ -f "$_so_path" ]; then
#             ld_library_path+=($(dirname $_so_path))
#             libs+=(-L$(dirname $_so_path))
#             # TODO: also update LD_LIBRARY_PATH
#         fi

#         # if [ -L "$_so_path" ]; then
#         #     libs+=(-L$(dirname $(readlink $_so_path)))
#         #     ld_library_path+=($(dirname $(readlink $_so_path)))
#         # fi
#     done

#     packages+=($pkg)
# }

# # install_one_package noavailpackage
# # install_one_package libhiredis-dev "$install_prefix" "$install_prefix"

# function install_package
# {
#     local pkg

#     for pkg in "$@"
#     do
#         echo $pkg
#         # install_one_package "$pkg" "$install_prefix" "$install_prefix"
#     done
# }

# # +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# # system packages, install by apt-get
# # cmake pkg-config swig3.0 doxygen graphviz aspell libxml-simple-perl
# # +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

# # install_package libhiredis-dev libnl-3-dev libnl-genl-3-dev libnl-route-3-dev
# # install_package swig3.0 libpython2.7-dev
# # install_package googletest
# # install_package libhiredis0.13
# #                 ^^^^^^^^^^^^^^ ??
# # +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

# # TODO: PACKAGE_CFG shall pass from args
# PACKAGE_CFG="/mnt/d/New_Folder/sonic-swss-acl-dev/package.cfg"
# . "$PACKAGE_CFG"

# echo $PKG_DEPENDENCIES

# if [ ! -z ${PKG_DEPENDENCIES+x} ]; then
#     PKG_DEPENDENCIES=($PKG_DEPENDENCIES)
#     install_package "${PKG_DEPENDENCIES[@]}"
# fi

# exit 1

# # +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# install_package libnl-3-dev libnl-genl-3-dev libnl-route-3-dev libhiredis-dev
# install_package libpython2.7-dev        # dev:test
# cflags+=(-I$(x86_64-linux-gnu-python2.7-config --prefix)/include)
# cflags+=($(x86_64-linux-gnu-python2.7-config --includes))

# install_package googletest              # dev:test
# # install_package doxygen graphviz # aspell # dev:doc
# # +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

# # rm pkg_config_path.txt
# # echo $PKG_CONFIG_PATH >> pkg_config_path.txt
# #
# # echo ${packages[@]}
# # for i in "${packages[@]}"; do echo "$i" ; done

# cat >"$install_prefix/.env" <<EOL
# export PKG_CONFIG_PATH=$PKG_CONFIG_PATH
# export PACKAGES="${packages[@]}"
# export LIBS="${libs[@]}"
# export CFLAGS="${cflags[@]}"
# export CXXFLAGS="${cflags[@]}"
# export _OLD_VIRTUAL_PATH="\$PATH"
# export PATH="$(join_by : ${path[@]}):\$PATH"
# export _OLD_LD_LIBRARY_PATH="\$LD_LIBRARY_PATH"
# export LD_LIBRARY_PATH="$(join_by : ${ld_library_path[@]}):\$LD_LIBRARY_PATH"
# EOL

# cat >"$install_prefix/.deenv" <<EOL
# if ! [ -z "\${_OLD_VIRTUAL_PATH+_}" ] ; then
#     PATH="\$_OLD_VIRTUAL_PATH"
#     export PATH
#     unset _OLD_VIRTUAL_PATH
# fi
# EOL

# # cat >add_to_profile ...