import socket
import csv
import time
import os

UDP_IP = "0.0.0.0"     
UDP_PORT = 5005      

filename = "csi_data_amp_phase.csv"
file_exists = os.path.isfile(filename)
is_empty = os.path.getsize(filename) == 0 if file_exists else True

csv_file = open(filename, mode='a', newline='')
csv_writer = csv.writer(csv_file)

if is_empty:
    header = ["Timestamp"]
    for i in range(64):
        header.append(f"Sub{i+1}_amp")
        header.append(f"Sub{i+1}_phase")
    header.append("Activity")
    csv_writer.writerow(header)
    csv_file.flush()

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening for UDP packets on port {UDP_PORT}...\nPress Ctrl+C to stop.\n")

try:
    while True:
        data, addr = sock.recvfrom(4096)
        line = data.decode().strip()

        pairs = line.split()
        if len(pairs) != 64:
            print(f"Skipped invalid packet (expected 64 pairs): {line}")
            continue

        row = [time.time()]

        try:
            for p in pairs:
                amp, phase = p.split(",")
                row.append(float(amp))
                row.append(float(phase))
        except ValueError:
            print(f"Invalid amp/phase format in packet: {line}")
            continue

        row.append("")
        csv_writer.writerow(row)
        csv_file.flush()

        print(f"Saved packet at timestamp {row[0]}")

except KeyboardInterrupt:
    print("\nReceiver stopped by user.")

except Exception as e:
    print(f"Error: {e}")

finally:
    csv_file.close()
    sock.close()
    print("Data saved to csi_data_amp_phase.csv")