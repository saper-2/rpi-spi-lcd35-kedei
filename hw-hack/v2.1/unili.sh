#!/bin/bash

error_str=""

echo -en "   Restoring tty2 mapping to fb0... "
con2fbmap 2 0 1>error_str 2>>error_str

if [ $? -ne 0 ]; then
	echo -e "\033[31mFAILED\033[1;31m:"
	echo $err_str
	echo -e "\033[0m"
	exit 1
fi

echo -e "\033[32mOK\033[0m"

echo -en "   Removing fbtft_device module from kernel... "

rmmod -f fbtft_device

if [ $? -ne 0 ]; then
        echo -e "\n\033[31mRMMOD FAILED\033[0m"
        exit 2
fi

echo -e "\033[32mOK\033[0m"


