#!/bin/bash

# ******************* TEST 1
echo -e "\033[33mBuilding test #1: lcd-test\033[0m"
g++ -o lcd-test lcd_test.cpp
if [ $? -eq 0 ] ; then
	echo -e "    \033[1;032mOK.\033[0m"
else
	echo -e "    \033[1;031mError\033[0m"
fi


# ******************* TEST 2
echo -e "\033[33mBuilding test #2: lcd-test2\033[0m"
g++ -o lcd-test2 lcd_test2.cpp
if [ $? -eq 0 ] ; then
	echo -e "    \033[1;032mOK.\033[0m"
else
	echo -e "    \033[1;031mError\033[0m"
fi


# ******************* TEST 3
echo -e "\033[33mBuilding test #3: lcd-test3\033[0m"
g++ -o lcd-test3 lcd_test3.cpp
if [ $? -eq 0 ] ; then
	echo -e "    \033[1;032mOK.\033[0m"
else
	echo -e "    \033[1;031mError\033[0m"
fi


# ******************* TEST 4
echo -e "\033[33mBuilding test #4: lcd-test4\033[0m"
g++ -o lcd-test4 lcd_test4.cpp
if [ $? -eq 0 ] ; then
	echo -e "    \033[1;032mOK.\033[0m"
else
	echo -e "    \033[1;031mError\033[0m"
fi


# ******************* TEST BCM2835 C++
echo -e "\033[33mBuilding test: lcd-test-bcm[c++]\033[0m"
g++ -o lcd-test-bcmc++ lcd_test_bcm.cpp -lbcm2835
if [ $? -eq 0 ] ; then
	echo -e "    \033[1;032mOK.\033[0m"
else
	echo -e "    \033[1;031mError\033[0m"
fi


# ******************* TEST BCM2835 C
echo -e "\033[33mBuilding test: lcd-test-bcm[c]\033[0m"
gcc -std=gnu11 -D_POSIX_C_SOURCE=200809L -o lcd-test-bcmc lcd_test_bcm.c -lbcm2835
if [ $? -eq 0 ] ; then
	echo -e "    \033[1;032mOK.\033[0m"
else
	echo -e "    \033[1;031mError\033[0m"
fi


# ******************* TEST BCM2835 C++ 3 bytes transfer
echo -e "\033[33mBuilding test: lcd-test-bcm[c++] 3 bytes\033[0m"
g++ -o lcd-test-bcm3bc++ lcd_test_bcm3b.cpp -lbcm2835
if [ $? -eq 0 ] ; then
	echo -e "    \033[1;032mOK.\033[0m"
else
	echo -e "    \033[1;031mError\033[0m"
fi


