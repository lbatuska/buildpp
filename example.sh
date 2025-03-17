#!/bin/bash

musl-clang ./buildpp/build.cpp ./buildpp/example.cpp -o ./build -static
sed 's|#include "build.h"|#include "buildpp/build.h"|' ./buildpp/example.cpp >./build.cpp
