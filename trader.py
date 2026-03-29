import socket
import struct
import time
import random

packet_format = '<QQIIB'
UDP_IP = "127.0.0.1"
UDP_PORT = 12345
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("--- URUCHAMIAM SYMULATOR GIEŁDY Z ANULOWANIEM ---")

current_market_price = 150
order_id = 1
num_orders_to_send = 10000

active_orders = []

print(f"Generowanie {num_orders_to_send} akcji w toku...")

for i in range(num_orders_to_send):
    current_market_price += random.choice([-1, 0, 1])
    current_market_price = max(1, current_market_price)

    if random.random() < 0.10 and len(active_orders) > 0:
        cancel_id = random.choice(active_orders)
        active_orders.remove(cancel_id)

        payload = struct.pack(packet_format, cancel_id, 0, 0, 0, 0)
        sock.sendto(payload, (UDP_IP, UDP_PORT))

    else:
        is_buy = random.choice([0, 1])

        if is_buy:
            if random.random() < 0.2:
                order_price = current_market_price + random.randint(0, 2)
            else:
                order_price = current_market_price - random.randint(1, 5)
        else:
            if random.random() < 0.2:
                order_price = current_market_price - random.randint(0, 2)
            else:
                order_price = current_market_price + random.randint(1, 5)

        order_price = max(1, order_price)
        quantity = random.randint(1, 10) * 10

        active_orders.append(order_id)

        payload = struct.pack(packet_format, order_id, order_price, 0, quantity, is_buy)
        sock.sendto(payload, (UDP_IP, UDP_PORT))

        order_id += 1

    time.sleep(0.001)

print("--- SYMULACJA ZAKOŃCZONA ---")