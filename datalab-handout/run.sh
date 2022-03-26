#!/bin/bash

make clean
make btest
./dlc bits.c
./btest bits.c
