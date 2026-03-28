import pandas as pd
import matplotlib.pyplot as plt

print("Wczytywanie logów transakcji z silnika C++...")

# Wczytujemy plik CSV wygenerowany przez Twój silnik
df = pd.read_csv("trades_0.csv")

# Przeliczamy milisekundy na czytelny czas
df['timestamp'] = pd.to_datetime(df['timestamp'], unit='ms')

plt.figure(figsize=(12, 6))

# Rysujemy linię zmiany ceny
plt.plot(df['timestamp'], df['price'], label='Cena Rynkowa (AAPL)', color='blue', linewidth=1.5)

# Dodajemy kropki w miejscach transakcji (wielkość kropki = ilość akcji)
plt.scatter(df['timestamp'], df['price'], s=df['quantity'], color='red', alpha=0.5, label='Wolumen transakcji')

plt.title('Symulacja HFT: Przebieg ceny na podstawie mikrosekundowych transakcji')
plt.xlabel('Czas (Milisekundy)')
plt.ylabel('Cena ($)')
plt.legend()
plt.grid(True)
plt.show()