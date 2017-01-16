#!/bin/bash

## SPECIFY TRIMESH LOCATION HERE (and do a make there first)
TRIMESH_DIR=../../trimesh2-master
SOURCE_DIR=../src/tri_convert/

## COMPILE AND LINK DEFINITIONS
COMPILE="g++ -std=c++11 -g -c -O3 -I../src/tri_tools/include/ -I ${TRIMESH_DIR}/include/"
COMPILE_BINARY="g++ -std=c++11 -c -O3 -I../src/tri_tools/include/ -I ${TRIMESH_DIR}/include/ -D BINARY_VOXELIZATION"
LINK="g++ -std=c++11 -o tri_convert"
LINK_BINARY="g++ -std=c++11 -g -o tri_convert_binary"
LINK_OPTS="-L${TRIMESH_DIR}/lib.Linux64 -ltrimesh -fopenmp -static"

#############################################################################################

## CLEAN
echo "Cleaning old versions ..."
rm -f *.o
rm -f tri_convert
rm -f a.out

## BUILD BINARY VOXELIZATION VERSION
echo "Building binary voxelization version ..."
${COMPILE_BINARY} ${SOURCE_DIR}tri_convert.cpp
${LINK_BINARY} tri_convert.o ${LINK_OPTS}

## CLEAN
echo "Cleaning..."
rm *.o
rm a.out

## BUILD REGULAR VERSION
echo "Building regular version ..."
${COMPILE} ${SOURCE_DIR}tri_convert.cpp
${LINK} tri_convert.o ${LINK_OPTS}

echo "Done"
