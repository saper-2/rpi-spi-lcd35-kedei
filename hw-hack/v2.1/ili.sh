#!/bin/bash

echo -e "\033[1;31mDe-initializing HW-H@cked KeDei 3.5\" LCD...\033[0m"
./unili.sh

if [ $? -ne 0 ]; then
        echo -e "\033[33mWARNING: there were errors.\033[0m"
fi


echo -e "\n"
echo -e "\033[1;32mInitializing HW-H@cked KeDei 3.5\" LCD...\033[0m"
./ili-init.sh

if [ $? -ne 0 ]; then
        echo -ne "\n\n\033[31m!!!!!!!!!!!!! LCD INIT FAILED !!!!!!!!!!!!!\033[0m"
        echo -e "\n"
        exit 1
fi


echo -ne "   Waiting for register frame buffer in system"

for xyz in 1 2 3 4 5
do
	echo -ne "."
	sleep 1
done

#echo "Set console: con2fbmap [tty] [fb] " 
echo -ne "\n   Setting up KeDei LCD to tty2... "
con2fbmap 2 1

if [ $? -ne 0 ]; then
	echo -e "\033[31mFAILED.\033[0m\n"
	exit 2
fi
echo -e "\033[1;32mOK\033[0m"

echo -e "\n\n\033[1;32mLCD has should be initialized and set as tty2 terminal (Switch with: Ctrl+Alt+F2).\033[0m"
echo -e "\033[;36mInstall fbi (if still didn't: \033[1;36mapt-get install fbi\033[0;36m) and to display image:"
echo -e "   \033[1;36msudo fbi -d /dev/fb1 -T 2 -noverbose -once -t 5 <image-file>\033[0m"

