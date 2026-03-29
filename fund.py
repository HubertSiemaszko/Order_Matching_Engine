import socket
import struct

UDP_IP = "127.0.0.1"
UDP_PORT = 12346

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print("--- ALGOTRADER TERMINAL ---")
print(f"Nasluchiwanie na gieldowy feed ITCH (Port UDP: {UDP_PORT})...")

format_P = '<cQIQI'

format_A = '<cQIQcQI'

while True:
    data, addr = sock.recvfrom(1024)

    if len(data) == 25:
        msg_type, timestamp, symbol_id, price, qty = struct.unpack(format_P, data)
        msg_type = msg_type.decode('ascii')

        if msg_type == 'P':
            print(f"$$$ [TRADE] MATCH! Cena: {price}$ | Ilosc: {qty} szt.")

    elif len(data) == 34:
        msg_type, timestamp, symbol_id, order_id, is_buy, price, qty = struct.unpack(format_A, data)
        msg_type = msg_type.decode('ascii')
        is_buy = is_buy.decode('ascii')
        kierunek = "KUPNO" if is_buy == 'B' else "SPRZEDAZ"

        if msg_type == 'A':
            print(f"--> [ORDER] KSG DODALA: {kierunek} | ID: {order_id} | Cena: {price}$ | Ilosc: {qty} szt.")
        elif msg_type == 'D':
            print(f"XXX [ORDER] KSG USUNELA: {kierunek} | ID: {order_id} | Cena: {price}$ | Ilosc: {qty} szt.")