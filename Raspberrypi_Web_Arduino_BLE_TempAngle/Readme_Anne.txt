## https://www.arduino.cc/en/Reference/ArduinoBLE
##
## Connect Arduino to Raspberry via BLE
## https://www.element14.com/community/community/project14/nano-rama/blog/2020/04/12/ble-on-arduino-nano-33-ble-sense-fruit-identification


# 1) Install latest Raspberry Pi OS Lite

# 2) Copy WiFi wpa_supplicant.conf and ssh to the root partition


# 3) Putty login and Update Dependencies
sudo apt-get update
sudo apt-get install -y libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev
sudo apt-get install python-docutils

User = pi (default pass = raspberry)
Change Passwd to: Welkomja123Pi  [ passwd pi ]
                  
# 4) Install BlueZ on Raspberry pi
## https://learn.adafruit.com/install-bluez-on-the-raspberry-pi/installation  NOTE: Old version of BlueZ
## Check for LATEST BLUEZ


cd /home/pi
mkdir software
cd software
wget https://mirrors.edge.kernel.org/pub/linux/bluetooth/bluez-5.59.tar.gz
tar xvf bluez-5.59.tar.gz
cd bluez-5.59

./configure --enable-library
# make will take 20-30min to compile
make
sudo make install

systemctl status bluetooth
sudo systemctl start bluetooth
sudo systemctl enable bluetooth
sudo systemctl daemon-reload
sudo systemctl restart bluetooth
bluetoothctl -v



# 5) Install bleak lib of Python3
sudo apt-get install python3-pip
sudo pip3 install bleak


# 6) Start Arduino Nano 33 BLE and scan for BLE

sudo hciconfig hci0 up
sudo hcitool lescan

NOTE: if bluetooth device is down:
sudo hciconfig hci0 up
hciconfig -a
sudo hcitool lescan


# 7) Python script example
pip3 install bluepy
pip3 install colr

python3 /home/pi/software/BLEClient.py 6C:80:44:58:CA:1F



# 8) Install Apache2; 
sudo apt update
sudo apt install apache2 -y
sudo apt-get install mc
sudo mkdir /var/www/tmp
sudo mkdir /var/www/tmp/ArduinoNano33
sudo chown -R pi: /var/www/tmp
cd /var/www/html
# make softlink to data folder
sudo ln -s /var/www/tmp/ArduinoNano33 ArduinoNano33
# web root is /var/www/html 
# copy index.html and the js folder to /var/www/html

Open the router (e.g. http://192.168.178.1/) and add a fixed IP: http://192.168.178.71/

# 8) ======= connect to nano 33 BLE and get UUID's
pi@raspberrypi:~ $ sudo gatttool -I
[                 ][LE]> connect 6C:80:44:58:CA:1F
Attempting to connect to 6C:80:44:58:CA:1F
Connection successful
[6C:80:44:58:CA:1F][LE]> primary
attr handle: 0x0001, end grp handle: 0x0005 uuid: 00001800-0000-1000-8000-00805f9b34fb
attr handle: 0x0006, end grp handle: 0x0009 uuid: 00001801-0000-1000-8000-00805f9b34fb
attr handle: 0x000a, end grp handle: 0x0010 uuid: 0000181a-0000-1000-8000-00805f9b34fb
[6C:80:44:58:CA:1F][LE]> characteristics
handle: 0x0002, char properties: 0x02, char value handle: 0x0003, uuid: 00002a00-0000-1000-8000-00805f9b34fb
handle: 0x0004, char properties: 0x02, char value handle: 0x0005, uuid: 00002a01-0000-1000-8000-00805f9b34fb
handle: 0x0007, char properties: 0x20, char value handle: 0x0008, uuid: 00002a05-0000-1000-8000-00805f9b34fb
handle: 0x000b, char properties: 0x02, char value handle: 0x000c, uuid: 00002a6d-0000-1000-8000-00805f9b34fb
handle: 0x000d, char properties: 0x02, char value handle: 0x000e, uuid: 00002a6e-0000-1000-8000-00805f9b34fb
handle: 0x000f, char properties: 0x02, char value handle: 0x0010, uuid: 00002a6f-0000-1000-8000-00805f9b34fb
[6C:80:44:58:CA:1F][LE]>



Arduino nano 33 BLE: D82CCF957951A3C0500 6C:80:44:58:CA:1F
sudo gatttool -i hci0 -b 6C:80:44:58:CA:1F --char-write-req --handle=0x0013 --value="0x50726F6A6563743134"  


