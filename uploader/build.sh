#!/bin/bash
rm -rf ./bin
mkdir -p bin
gcc -g -static -O3 -lpthread -pthread src/*.c -o bin/uploader

cp -avr ../download/bin/* ./bin/
