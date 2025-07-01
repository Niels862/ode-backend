import serial.tools.list_ports


def select_port():
    ports = serial.tools.list_ports.comports()
    usb_ports = []

    for port in ports:
        print(port.name, port.description)


def main():
    select_port()


if __name__ == "__main__":
    main()
