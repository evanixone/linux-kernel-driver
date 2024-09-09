#!/bin/bash

COMMAND=$1

if [ $COMMAND -eq 1 ]; then
	make
	sudo insmod temp_driver.ko
	sudo insmod display_driver.ko
else
	make clean
	sudo rmmod temp_driver.ko
	sudo rmmod display_driver.ko
fi

