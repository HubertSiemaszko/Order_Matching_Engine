#include <cstdint>
#include <deque>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <functional>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
const size_t MAX_PRICE_LEVELS = 1000000;
struct Order {
    unsigned long long int symbolId;
    unsigned long long int OrderId;
    unsigned long long int Price;
    unsigned long int Quantity;
    bool isBuy;
    bool isActive;
    Order() : OrderId(0), Price(0), Quantity(0), isBuy(false) {}
    Order(unsigned long long int id, unsigned long long int p, unsigned long int q, bool buy)
        : OrderId(id), Price(p), Quantity(q), isBuy(buy) {}

};

struct Level {
    unsigned long long int totalQuantity = 0;
    std::deque<Order> orders;
};

struct OrderLocation {
    unsigned long long int price;
    bool isBuy;
    size_t index;
};

struct PriceLevelInfo {
    std::vector<Order> orders;
    unsigned long long int Quantity;
    Order* firstOrder;
    Order* lastOrder;
};

class OrderBook {
    public:
    OrderBook() : asks(MAX_PRICE_LEVELS), bids(MAX_PRICE_LEVELS) {}
    void addOrder(Order newOrder) {
        if (newOrder.Price>=MAX_PRICE_LEVELS) {
            return;
        }
        if (newOrder.isBuy) {
            auto& vec = bids[newOrder.Price];
            vec.push_back(newOrder);
            idToOrder[newOrder.OrderId] = { newOrder.Price, true, vec.size() - 1 };
            //std::cout<<"Dodano zlecenie Kupna:"<<newOrder.OrderId<<std::endl;
            //std::cout<<"Wartosc:"<<newOrder.Price<<std::endl;
            //std::cout<<"Ilosc:"<<newOrder.Quantity<<std::endl;
            if (newOrder.Price>=bestBid) {
                bestBid = newOrder.Price;
            }
        }

        else {
            auto& vec = asks[newOrder.Price];
            vec.push_back(newOrder);
            idToOrder[newOrder.OrderId] = { newOrder.Price, false, vec.size() - 1 };
            //std::cout<<"Dodano zlecenie Sprzedaży:"<<newOrder.OrderId<<std::endl;
            //std::cout<<"Wartosc:"<<newOrder.Price<<std::endl;
            //std::cout<<"Ilosc:"<<newOrder.Quantity<<std::endl;
            if (newOrder.Price>=bestAsk) {
                bestAsk = newOrder.Price;
            }
        }
        matchOrders();
    }

    void matchOrders() {
        if (asks.empty() || bids.empty()) return;

        while (bestBid>=bestAsk&&(!asks.empty()||!bids.empty())&& bestAsk < MAX_PRICE_LEVELS && bestBid > 0) {
            auto askVec = asks[bestAsk];
            auto bidVec = bids[bestBid];

            // Znajdź pierwsze aktywne zlecenie po stronie ASK
            size_t askIdx = 0;
            while (askIdx < askVec.size() && (!askVec[askIdx].isActive || askVec[askIdx].Quantity == 0)) {
                askIdx++;
            }

            // Znajdź pierwsze aktywne zlecenie po stronie BID
            size_t bidIdx = 0;
            while (bidIdx < bidVec.size() && (!bidVec[bidIdx].isActive || bidVec[bidIdx].Quantity == 0)) {
                bidIdx++;
            }

            bool levelChanged = false;
            if (askIdx==askVec.size()) {
                if (bestAsk==MAX_PRICE_LEVELS) {
                    break;
                }
                levelChanged = true;
                askVec.clear();
                bestAsk++;
            }
            if (bidIdx==bidVec.size()) {
                if (bestBid==0) {
                    break;
                }
                levelChanged = true;
                askVec.clear();
                bestAsk--;
            }

            if (levelChanged) continue;

            Order& sellOrder = askVec[askIdx];
            Order& buyOrder = bidVec[bidIdx];

            unsigned long quantityToTrade = std::min(sellOrder.Quantity, buyOrder.Quantity);

            sellOrder.Quantity -= quantityToTrade;
            buyOrder.Quantity -= quantityToTrade;

            if (sellOrder.Quantity == 0) {
                sellOrder.isActive = false;
                idToOrder.erase(sellOrder.OrderId);
            }

            if (buyOrder.Quantity == 0) {
                buyOrder.isActive = false;
                idToOrder.erase(buyOrder.OrderId);
            }
        }
    }

    void printBook() {

        std::cout<<"ASKS"<<std::endl;
        for (size_t i=bestAsk; i<MAX_PRICE_LEVELS; i++ )
        {
            if (!asks[i].empty()) {
                for (auto const& order:asks[i]) {
                    if (order.isActive) {
                        std::cout<<order.OrderId<<" "<<order.Price<<" "<<order.Quantity<<std::endl;
                    }
                }
            }
        }
        std::cout<<"BIDS"<<std::endl;
        for (size_t i=bestBid; i>0; i--)
        {
            for (auto const& order:bids[i]) {
                if (order.isActive) {
                    std::cout<<order.OrderId<<" "<<order.Price<<" "<<order.Quantity<<std::endl;
                };
            }
        }

    }


    void cancelOrder(unsigned long long int id){
        auto findId = idToOrder.find(id);
        if (findId == idToOrder.end()){
            return;
        }

        OrderLocation loc = findId->second;

        if (loc.isBuy) {
            bids[loc.price][loc.index].isActive = false;
        } else {
            asks[loc.price][loc.index].isActive = false;
        }

        idToOrder.erase(id);
    }
    private:
        std::vector<std::vector<Order>> asks; //from smallest to biggest
        std::vector<std::vector<Order>> bids; //from biggest to smallest
        std::unordered_map<unsigned long long int, OrderLocation> idToOrder;
        unsigned long long int bestBid = 0;
        unsigned long long int bestAsk = MAX_PRICE_LEVELS;
};

class OrderBookThread {
private:
    OrderBook book;
    std::queue<Order> incomingOrders;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> running{true};
    std::thread workerThread;

    void processLoop() {
        while (running) {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return !incomingOrders.empty()||!running; });
            if (!running&&incomingOrders.empty()) break;

            Order order=incomingOrders.front();
            incomingOrders.pop();
            lock.unlock();
            book.addOrder(order);
        }
    }
public:
    OrderBookThread() {
        workerThread = std::thread(&OrderBookThread::processLoop, this);
    }

    ~OrderBookThread() {
        running = false;
        cv.notify_one();
        if (workerThread.joinable()) workerThread.join();
    }

    void submitOrder(Order ord) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            incomingOrders.push(ord);
        }
        cv.notify_one();
    }
};

class ExchangeDispatcher {
private:
    std::unordered_map<unsigned long long int, std::unique_ptr<OrderBookThread>> shards;

public:
    void addOrder(std::string_view symbol, Order ord) {
        ord.symbolId = getInternalId(symbol);

        auto it = shards.find(ord.symbolId);

        if (it == shards.end()) {
            shards[ord.symbolId] = std::make_unique<OrderBookThread>();
            it = shards.find(ord.symbolId);
        }

        it->second->submitOrder(std::move(ord));
    }
    unsigned int getInternalId(std::string_view symbol) {
        static std::unordered_map<std::string, unsigned int> registry;
        static unsigned int nextId = 0;

        auto it = registry.find(std::string(symbol));
        if (it == registry.end()) {
            registry[std::string(symbol)] = ++nextId;
            return nextId;
        }
        return it->second;
    }
};

#include <chrono>
#include <iomanip>

int main() {
    const int NUM_ORDERS = 1000000;
    ExchangeDispatcher dispatcher;

    // Przygotowanie danych testowych
    std::vector<Order> testOrders;
    for (int i = 0; i < NUM_ORDERS; ++i) {
        Order o(i, 100 + (i % 10), 10, (i % 2 == 0));
        testOrders.push_back(o);
    }

    std::cout << "Rozpoczynam benchmark dla " << NUM_ORDERS << " zlecen..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ORDERS; ++i) {
        std::string_view symbol = (i % 2 == 0) ? "AAPL" : "TSLA";

        dispatcher.addOrder(symbol, testOrders[i]);
    }

    // Uwaga: Dispatcher działa asynchronicznie (wątki).
    // W profesjonalnym teście musielibyśmy poczekać, aż kolejki będą puste.
    // Dla uproszczenia testujemy tutaj szybkość samego Dispatchera i wrzucania do kolejek.

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    double ops = NUM_ORDERS / diff.count();

    std::cout << "--- WYNIKI BENCHMARKU ---" << std::endl;
    std::cout << "Czas calkowity: " << std::fixed << std::setprecision(4) << diff.count() << " s" << std::endl;
    std::cout << "Przepustowosc: " << std::fixed << std::setprecision(0) << ops << " zlecen/sekunda" << std::endl;

    return 0;
}