PROJECT_ROOT="$(dirname "$BASH_SOURCE")"

if [[ ! -f "${PROJECT_ROOT}/esp-idf/export.sh" ]]
then
    echo "esp-idf is missing, please check out all needed submodules!"
    echo "git submodule update --init --recursive"
    return
fi

export PATH=$PATH:$(pwd)/tools

. ${PROJECT_ROOT}/esp-idf/export.sh
