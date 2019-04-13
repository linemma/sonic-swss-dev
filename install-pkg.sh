function install_pkg_main
{
    local _DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

    local _pkg_scripts_home="$_DIR"
    local _package_cfg_path="$_DIR/package.cfg"
    local _build_home="$(pwd)"

    echo pkg_scripts_home=$_pkg_scripts_home
    echo package_cfg_path=$_package_cfg_path
    echo build_home=$_build_home

    local pi_options=()

    while [[ $# -ne 0 ]]
    do
        arg="$1"
        case "$arg" in
            -g|--only-global)
                pi_options+="-g"
                ;;
            *)
                echo >&2 "Invalid option \"$arg\""
                exit 1
        esac
        shift
    done

    . $_DIR/package-installer/cli.sh ${pi_options[@]} --plugins-dir="$_pkg_scripts_home" --destination-dir="$_build_home" "$_package_cfg_path"
}

install_pkg_main "$@"