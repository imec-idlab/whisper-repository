#!/bin/bash


sudo apt-get update
mkdir openwsn
sudo apt-get install -y git
cd ./openwsn/
git clone https://github.com/openwsn-berkeley/openwsn-fw.git
git clone https://github.com/openwsn-berkeley/openwsn-sw.git
git clone https://github.com/openwsn-berkeley/coap.git
cd ./openwsn-fw/
sudo apt-get install -y python-dev
sudo apt-get install -y scons
cd ../openwsn-sw/
sudo apt-get install -y python-pip
sudo apt-get install -y python-tk
sudo pip install -r requirements.txt
cd ../coap/
sudo pip install -r requirements.txt
sudo apt-get install -y gcc-arm-none-eabi
sudo apt-get install -y gcc-msp430

