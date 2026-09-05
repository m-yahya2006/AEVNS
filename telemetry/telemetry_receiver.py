import os
import socket
import csv
from datetime import datetime

script_dir = os.path.dirname(os.path.abspath(__file__))
file_path = os.path.join(script_dir, "RobotData.csv")

def parse_sensor_data(raw):
    try:
        parts = [p.strip() for p in raw.split(",")]
        if len(parts)!=16:
            return None
        return [
            float(parts[0]),
            float(parts[1]),
            float(parts[2]),
            float(parts[3]),
            float(parts[4]),
            float(parts[5]),
            float(parts[6]),
            parts[7],
            float(parts[8]),
            float(parts[9]),
            float(parts[10]),
            parts[11],
            parts[12],
            parts[13],
            parts[14],
            int(parts[15]),
        ]    
    except ValueError:
        return None    

file_is_new_or_empty = not os.path.exists(file_path) or os.path.getsize(file_path)==0

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 4210))

CSV_HEADERS = [
    "timestamp",
    "battery_voltage",
    "battery_current",
    "battery_power",
    "battery_soc",
    "battery_temperature",
    "solar_voltage",
    "solar_current",
    "solar_active",
    "left_distance",
    "right_distance",
    "front_distance",
    "left_zone",
    "right_zone",
    "front_zone",
    "action",
    "drive_speed",
]

if file_is_new_or_empty:
    with open(file_path, mode="a", newline="" ) as f:
        writer = csv.writer(f)
        writer.writerow(CSV_HEADERS) 

try:
    while True:
    
        data, address = sock.recvfrom(1024)
        message = data.decode().strip()
        result = parse_sensor_data(message)
        if result is None:
            print("Invalid data",message)
            continue
        timestamp = datetime.now().isoformat(timespec = "seconds")
        row = [timestamp] + result

        with open(file_path, mode="a",newline="") as f:
            writer= csv.writer(f)
            writer.writerow(row)

        print("Row saved", row)

except KeyboardInterrupt:
    print("Reciever stopped")

finally:
    sock.close()    
