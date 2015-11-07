#! /bin/bash

g++ --std=c++11 -c mandelbrot.cpp viewer.cpp
g++ --std=c++11 viewer.o mandelbrot.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system
