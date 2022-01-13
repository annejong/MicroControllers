import sys
import datetime
import time
from argparse import ArgumentParser

from bluepy import btle  # linux only (no mac)
from colr import color as colr


# BLE IoT Sensor Demo
# Author: Gary Stafford
# Reference: https://elinux.org/RPi_Bluetooth_LE
# Requirements: python3 -m pip install –user -r requirements.txt
# To Run: python3 ./rasppi_ble_receiver.py d1:aa:89:0c:ee:82 <- MAC address – change me!


def main():
	# get args
	args = get_args()
	
	deltaTemp = 0.2 ;
	
	print("Connecting…")
	nano_sense = btle.Peripheral(args.mac_address)
	
	print("Discovering Services…")
	_ = nano_sense.services
	environmental_sensing_service = nano_sense.getServiceByUUID("181A")
	
	print("Discovering Characteristics…")
	_ = environmental_sensing_service.getCharacteristics()
	
	my_table =  {"time":[],"temp":[],"hum":[],"pres":[]};
	prev_temperature = 0 ;
	time_start = datetime.datetime.now()
	print("time\ttemp\thumidity\tpressure")
	firstlog = True
	while True:
		temperature = read_temperature(environmental_sensing_service)
		humidity = read_humidity(environmental_sensing_service)
		pressure = read_pressure(environmental_sensing_service)
		# color = read_color(environmental_sensing_service)
		degreeX = read_value(environmental_sensing_service, "2201")
		degreeY = read_value(environmental_sensing_service, "2202")
		time_diff = ( datetime.datetime.now() - time_start )
		time_minutes = round(time_diff.total_seconds() / 1)
		time.sleep(5) # transmission frequency set on IoT device
		print(f"sense={time_minutes}|{temperature}")
		# update file only if value are changed more than deltaTemp since last log
		if ( abs(temperature - prev_temperature) >= deltaTemp or firstlog):
			firstlog = False
			print(f"{time_minutes}\t{temperature}\t{humidity}\t{pressure}")
			my_table["time"].append(time_minutes)
			my_table["temp"].append(temperature)
			my_table["hum"].append(humidity)
			my_table["pres"].append(pressure)
			prev_temperature = temperature
			update_table(my_table) 
			

def read_value(service, uuid):
	value_char = service.getCharacteristics(uuid)[0]
	value = value_char.read()
	value = byte_array_to_int(value) /10
	value = round(value, 1)  
	return value
			


def update_table(my_table):
	header = '\t'.join(list(my_table.keys()))
	lines = [header]
	for i in range(len(my_table["temp"])):
		row = []
		for key in my_table: row.append(my_table[key][i])
		lines.append('\t'.join(str(x) for x in row))
	with open('/var/www/tmp/ArduinoNano33/ArduinoNano33.table', 'w') as f: f.writelines('\n'.join(lines))	
	return 1

def byte_array_to_int(value):
    # Raw data is hexstring of int values, as a series of bytes, in little endian byte order
    # values are converted from bytes -> bytearray -> int
    # e.g., b'\xb8\x08\x00\x00' -> bytearray(b'\xb8\x08\x00\x00') -> 2232
    # print(f"{sys._getframe().f_code.co_name}: {value}")
    value = bytearray(value)
    value = int.from_bytes(value, byteorder="little")
    return value


def split_color_str_to_array(value):
    # e.g., b'2660,2059,1787,4097\x00' -> 2660,2059,1787,4097 ->
    #       [2660, 2059, 1787, 4097] -> 166.0,128.0,111.0,255.0

    # print(f"{sys._getframe().f_code.co_name}: {value}")

    # remove extra bit on end ('\x00')
    #value = value[0:–1]

    # split r, g, b, a values into array of 16-bit ints
    values = list(map(int, value.split(",")))

    # convert from 16-bit ints (2^16 or 0-65535) to 8-bit ints (2^8 or 0-255)
    # values[:] = [int(v) % 256 for v in values]

    # actual sensor is reading values are from 0 – 4097
    print(f"12-bit Color values (r,g,b,a): {values}")
    values[:] = [round(int(v) / (4097 / 255), 0) for v in values]
    return values


def byte_array_to_char(value):
    # e.g., b'2660,2058,1787,4097\x00' -> 2659,2058,1785,4097
    value = value.decode("utf-8")
    return value


def decimal_exponent_two(value):
    # e.g., 2350 -> 23.5
    return value / 100


def decimal_exponent_one(value):
    # e.g., 988343 -> 98834.3
    return value / 10


def pascals_to_kilopascals(value):
    # 1 Kilopascal (kPa) is equal to 1000 pascals (Pa)
    # to convert kPa to pascal, multiply the kPa value by 1000
    # 98834.3 -> 98.8343
    return value / 1000


def celsius_to_fahrenheit(value):
    return (value * 1.8) + 32


def read_color(service):
    color_char = service.getCharacteristics("936b6a25-e503-4f7c-9349-bcc76c22b8c3")[0]
    color = color_char.read()
    color = byte_array_to_char(color)
    color = split_color_str_to_array(color)
    print(f" 8-bit Color values (r,g,b,a): {color[0]},{color[1]},{color[2]},{color[3]}")
    print("RGB Color")
    print(colr('\t\t', fore=(127, 127, 127), back=(color[0], color[1], color[2])))
    print("Light Intensity")
    print(colr('\t\t', fore=(127, 127, 127), back=(color[3], color[3], color[3])))


def read_pressure(service):
	pressure_char = service.getCharacteristics("2A6D")[0]
	pressure = pressure_char.read()
	pressure = byte_array_to_int(pressure)
	pressure = decimal_exponent_one(pressure)
	pressure = pascals_to_kilopascals(pressure)
	pressure = round(pressure, 1)
	#print(f"Barometric Pressure: {pressure)} kPa")
	return pressure


def read_humidity(service):
	humidity_char = service.getCharacteristics("2A6F")[0]
	humidity = humidity_char.read()
	humidity = byte_array_to_int(humidity)
	humidity = decimal_exponent_two(humidity)
	humidity = round(humidity, 0)
	# print(f"Humidity: {round(humidity, 0)}%")
	return humidity


def read_temperature(service):
	temperature_char = service.getCharacteristics("2A6E")[0]
	temperature = temperature_char.read()
	temperature = byte_array_to_int(temperature)
	temperature = decimal_exponent_two(temperature)
	#temperature = celsius_to_fahrenheit(temperature)
	temperature = round(temperature - 1.5, 1)  # plus correction of 1.5°C
	#print(f"Temperature: {temperature}°C")
	return temperature


def get_args():
    arg_parser = ArgumentParser(description="BLE IoT Sensor Demo")
    arg_parser.add_argument('mac_address', help="MAC address of device to connect")
    args = arg_parser.parse_args()
    return args


if __name__ == "__main__":
    main()
	
	