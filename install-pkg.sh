_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

_pkg_scripts_home=${1:-$_DIR}
_package_cfg_path=${2:-"$_DIR"/package.cfg}
_build_home=${3:-$(pwd)}

_script_home=$_pkg_scripts_home

# echo pkg_scripts_home=$_pkg_scripts_home
# echo package_cfg_path=$_package_cfg_path
# echo build_home=$_build_home

. $_DIR/package-installer/cli.sh "$_pkg_scripts_home" "$_package_cfg_path" "$_build_home"
