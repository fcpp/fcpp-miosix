#!/bin/bash

if [ "$1" == "" ]; then
    echo -e "\033[4musage:\033[0m"
    echo -e "    \033[1m./make.sh [platform] [target...]\033[0m"
    #echo -e "you can specify \"all\" as target to make all targets."
    exit 1
fi

platform=$1
if [ "$platform" == windows ]; then
    flag=MinGW
elif [ "$platform" == unix ]; then
    flag=Unix
else
    echo -e "\033[4mUnrecognized platform \"$platform\". Available platforms are:\033[0m"
    echo -e "    \033[1mwindows unix\033[0m"
    exit 1
fi
shift 1

function make_target {
    target=$1
    if [ "$target" == "simulation" ]; then
        cmake -S ./ -B ./bin -G "$flag Makefiles" -DCMAKE_BUILD_TYPE=Release -Wno-dev
        cmake --build ./bin/
        if [ "$platform" == windows ]; then
            cp bin/fcpp/src/libfcpp.dll bin/
        fi
        cd bin
        ./miosix_simulation | tee ../plot/simulation.asy
        cd ../plot
        asy simulation.asy -f pdf
        rm *tim*
        cd ..
    else
        echo -e "\033[4mUnrecognized target \"$target\".\033[0m"
        exit 1
    fi
}

git submodule init
git submodule update
if [ "$1" == "all" ]; then
    for target in simulation; do
        make_target $target
    done
else
    for target in "$@"; do
        make_target $target
    done
fi
