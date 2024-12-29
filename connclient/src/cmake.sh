#!/bin/sh

if [ $# -gt 0 ]; then
    flag=$1
else
    flag="default"
fi

if [ "$flag" = "all" ]; then
    mkdir -p build && rm -rf ./build/* && rm -rf ./lib/*
    cd build && cmake ../
    if ! make -j8; then
        echo "make error!"
        exit 1
	else 
        echo "make success!"
    fi
else
    cd ./build || exit
    if ! make -j8; then
        echo "make error!"
        exit 1
	else
        echo "make success."
    fi
fi
