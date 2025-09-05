import socket
import numpy as np
import joblib
import time
from collections import Counter
import re

model = joblib.load("model.pkl")
scaler = joblib.load("scaler.pkl")

label_map = {
    0: 'Empty',
    1: 'Sit',
    2: 'Sleeping',
    3: 'Stand'
}

UDP_IP = "0.0.0.0"
UDP_PORT = 5005

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on UDP {UDP_IP}:{UDP_PORT}...\n")

predictions_in_window = []
start_time = time.time()

while True:
    try:
        data, addr = sock.recvfrom(8192)
        data_str = data.decode(errors='ignore').strip()

        cleaned_str = re.sub(r"[,\t]+", " ", data_str)
        values = [float(x) for x in cleaned_str.split() if x.strip() != ""]

        if len(values) >= 129:
            features = values[1:]
        else:
            features = values

        if len(features) != 128:
            print(f"Warning: Expected 128 features, got {len(features)}. Skipping this packet.")
            continue

        X = np.array(features).reshape(1, -1)
        X_scaled = scaler.transform(X)

        pred_label = model.predict(X_scaled)[0]
        predictions_in_window.append(pred_label)

        if time.time() - start_time >= 0.2:
            if predictions_in_window:
                most_common_label, count = Counter(predictions_in_window).most_common(1)[0]
                activity = label_map.get(most_common_label, "Unknown")
                print(f"Final Prediction: {activity}")
            predictions_in_window = []
            start_time = time.time()

    except Exception as e:
        print("Error:", e)
