#!/bin/bash

if [ "$1" == "" ]; then
    echo -e "\033[4musage:\033[0m"
    echo -e "    \033[1m./make.sh [platform]\033[0m"
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

git submodule init
git submodule update
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
