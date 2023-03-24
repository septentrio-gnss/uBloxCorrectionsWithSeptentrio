import time
import argparse
import serial # pip install pyserial
import socket
#python3 reseter.py --main_comm USB --main_config /dev/ttyACM0@115200
def main():
    ap = argparse.ArgumentParser()

    # Main comm channel config
    ap.add_argument('--main_comm', required=True, help='main_comm: [USB or IP]')
    ap.add_argument('--main_config', required=True, help='if Main USB: port@baudrate / if Main IP: address@port')

    args = ap.parse_args()

    cmd_mode = 'SSSSSSSSSSSSSSSSSSSSSSS\x0D'
    cmd_reset = 'erst, soft, Config\x0D'

    # Set main comm chanel
    main_comm_name = args.main_comm

    if (main_comm_name == 'USB'):
        main_serial_params = args.main_config.split('@')
        (main_serial_port, main_serial_baud) = (main_serial_params[0], int(main_serial_params[1]))

        # Create Main serial port object
        main_channel = serial.Serial(port=main_serial_port, baudrate = main_serial_baud, timeout =0.1) 

        main_channel.write(bytes(cmd_mode, 'utf-8'))
        time.sleep(2)
        main_channel.write(bytes(cmd_reset, 'utf-8'))

    if (main_comm_name == 'IP'):
        main_IP_params = args.main_config.split('@')
        (main_IP_address, main_IP_port) = (main_IP_params[0], int(main_IP_params[1]))

        # Create Main IP object
        main_channel = socket.create_connection((main_IP_address, main_IP_port), timeout=0.1)

        # Detect main channel IP Port name
        main_channel_port_name = detectPortIp(main_channel)

        main_channel.send(bytes(cmd_mode, 'utf-8'))
        time.sleep(2)
        main_channel.send(bytes(cmd_reset, 'utf-8'))

if __name__ == '__main__':
    main()