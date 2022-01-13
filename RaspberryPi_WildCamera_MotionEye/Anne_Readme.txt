Wild camera met raspberry pi en MotionEye

1. MakerHawk Raspberry Pi 4B Camera met houder en kabel IR Camera Module 5MP OV5647 Webcam automatisch schakelen tussen dag en nacht visio voor Raspberry
2. RaspberryPi zeroW

https://randomnerdtutorials.com/install-motioneyeos-on-raspberry-pi-surveillance-camera-system/

Installation

1. Downloading the OS Image from https://github.com/ccrisan/motioneyeos/releases
	- motioneyeos-raspberrypi-20200606.img.xz
2. Use Etcher (balenaEtcher) to brun the image to SD
	- https://www.balena.io/etcher/
	
3. After burning add the file wpa_supplicant.conf and ssh to the SD card	
	
wpa_supplicant.conf content=
"
country=NL
update_config=1
ctrl_interface=/var/run/wpa_supplicant

network={
	ssid="Hartelijk"
	psk="Welkomja123"
	priority=1
    id_str="Huis"
}


network={
	ssid="Klasien-Veranda"
	psk="Welkomja123"
	priority=2
    id_str="Veranda"
}

"

4. Eject SD and insert into Raspberry pi zeroW

5. Open home modem : http://192.168.178.1/
	- Find MAC-adres at connected devices (device name = meye-987318cf)
	In my case this is: B8:27:EB:26:4D:9A
	
	- At DHCP setting set a fixed IP for this Mac-Adres
	In my case this is: 192.168.178.62
	
6. Install MotionEye on android
	first login "admin" no pass
	
	In setting:
	Still Images: "motion triggered" , "quality 100%"
	Motion detection: "Auto Threshold" "Minimum Motion Frames=1" 
	
	
	
	