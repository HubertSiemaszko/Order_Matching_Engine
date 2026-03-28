import socket
import struct
import time
import random

packet_format = '<QQIIB'
UDP_IP = "127.0.0.1"
UDP_PORT = 12345
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print("--- URUCHAMIAM SYMULATOR GIEŁDY ---")

# Startujemy od ceny 150$ (używamy liczb całkowitych jak w HFT)
current_market_price = 150
order_id = 1
num_orders_to_send = 10000 # Strzelamy 10 tysiącami zleceń!

print(f"Generowanie {num_orders_to_send} zlecen w toku...")

for i in range(num_orders_to_send):
    # 1. Spacer losowy ceny rynku (Cena lekko drży w górę lub w dół)
    current_market_price += random.choice([-1, 0, 1])

    # 2. Losujemy: 1 = KUPNO, 0 = SPRZEDAŻ
    is_buy = random.choice([0, 1])

    # 3. Mechanika Spreadu i Transakcji
    # Jeśli kupujemy, zazwyczaj chcemy kupić taniej (poniżej current_market_price)
    # Czasami (w 20% przypadków) dajemy cenę AGRESYWNĄ (przepłacamy), żeby wywołać MATCH!
    if is_buy:
        if random.random() < 0.2:
            order_price = current_market_price + random.randint(0, 2) # Agresywny kupiec
        else:
            order_price = current_market_price - random.randint(1, 5) # Czeka na okazję
    else:
        if random.random() < 0.2:
            order_price = current_market_price - random.randint(0, 2) # Agresywny sprzedawca (panika)
        else:
            order_price = current_market_price + random.randint(1, 5) # Czeka na zysk

    # Ilość od 10 do 100 akcji
    quantity = random.randint(1, 10) * 10

    # Pakujemy i wysyłamy UDP prosto do pamięci RAM Twojego C++
    payload = struct.pack(packet_format, order_id, order_price, 0, quantity, is_buy)
    sock.sendto(payload, (UDP_IP, UDP_PORT))

    order_id += 1

    # Minimalne opóźnienie (0.001s), żebyś mógł nacieszyć oczy logami z MATCH-y.
    # W prawdziwym benchmarku usunąłbyś tego sleepa całkowicie!
    time.sleep(0.001)

print("--- SYMULACJA ZAKOŃCZONA ---")