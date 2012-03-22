#!/bin/bash
#
wifi_3g_version="$1"
CPU_JOB_NUM=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print field+1}')

START_TIME=`date +%s`
wifi_3g_version=`echo "$wifi_3g_version" | tr '[:upper:]' '[:lower:]'`

#make distclean
if [ "$wifi_3g_version" == "3g" ]; then
  echo "****************Build 3g version*******************"     
else 
  echo "****************Build wifi version*****************"    
fi
if [ "$wifi_3g_version" == "3g" ]; then
  echo " " 
  make omap3621_cl1_3g_defconfig  
  #sed -i "314a CONFIG_USB_SERIAL_SIERRAWIRELESS=y\nCONFIG_CL1_3G_BOARD=y" .config
else 
  echo " "  
  make omap3621_cl1_defconfig  
  #sed -i 's/CONFIG_USB_SERIAL_SIERRAWIRELESS=y/CONFIG_USB_SERIAL_SIERRAWIRELESS=n/g' .config
fi
make uImage -j$CPU_JOB_NUM
#make modules
END_TIME=`date +%s`
let "ELAPSED_TIME=$END_TIME-$START_TIME"

ls arch/arm/boot/uImage &> /dev/null
if [ $? != 0 ]; then
  echo "Total kernel built time = $ELAPSED_TIME seconds, FAIL !!"
else
  echo "Total kernel built time = $ELAPSED_TIME seconds, Success"
fi
