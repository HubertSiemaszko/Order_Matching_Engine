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
struct Order {
    std::string symbol;
    unsigned long long int OrderId;
    unsigned long long int Price;
    unsigned long int Quantity;
    bool isBuy;
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
    std::list<Order>::iterator it;
};

class OrderBook {
    public:
    void addOrder(Order newOrder) {
        if (newOrder.isBuy) {
            //bids[newOrder.Price].push_back(newOrder);
            auto& lista = bids[newOrder.Price];
            lista.push_back(newOrder);
            idToOrder[newOrder.OrderId] = { newOrder.Price, newOrder.isBuy, std::prev(lista.end()) };
            std::cout<<"Dodano zlecenie Kupna:"<<newOrder.OrderId<<std::endl;
            std::cout<<"Wartosc:"<<newOrder.Price<<std::endl;
            std::cout<<"Ilosc:"<<newOrder.Quantity<<std::endl;
        }
        else {
            //asks[newOrder.Price].push_back(newOrder);
            auto& lista = asks[newOrder.Price];
            lista.push_back(newOrder);
            idToOrder[newOrder.OrderId] = { newOrder.Price, newOrder.isBuy, std::prev(lista.end()) };
            std::cout<<"Dodano zlecenie Sprzedaży:"<<newOrder.OrderId<<std::endl;
            std::cout<<"Wartosc:"<<newOrder.Price<<std::endl;
            std::cout<<"Ilosc:"<<newOrder.Quantity<<std::endl;
        }
        matchOrders();
    }

    void matchOrders(){
        if (asks.empty()||bids.empty()) {return;}
        while (!asks.empty() && !bids.empty() && bids.begin()->first >= asks.begin()->first) {
            auto itAsk = asks.begin();
            auto itBid = bids.begin();
            Order& sellOrder = itAsk->second.front();
            Order& buyOrder = itBid->second.front();

            unsigned long quantityToTrade = std::min(sellOrder.Quantity, buyOrder.Quantity);

            std::cout << "TRANSAKCJA: " << quantityToTrade << " sztuk po cenie " << itAsk->first << std::endl;

            sellOrder.Quantity -= quantityToTrade;
            buyOrder.Quantity -= quantityToTrade;

            if (sellOrder.Quantity == 0) {
                idToOrder.erase(sellOrder.OrderId);
                itAsk->second.pop_front();
                if (itAsk->second.empty()) {
                    asks.erase(itAsk);
                }
            }

            if (buyOrder.Quantity == 0) {
                idToOrder.erase(buyOrder.OrderId);
                itBid->second.pop_front();
                if (itBid->second.empty()) {
                    bids.erase(itBid);
                }
            }

        }
    }

    void printBook() {

        std::cout<<"ASKS"<<std::endl;
        for (auto const& [price, queue] : asks)
        {
            for (auto const& order: queue) {
                std::cout<<order.OrderId<<" "<<order.Price<<" "<<order.Quantity<<std::endl;
            }
        }
        std::cout<<"BIDS"<<std::endl;
        for (auto const& [price, queue] : bids)
        {
            for (auto const& order: queue) {
                std::cout<<order.OrderId<<" "<<order.Price<<" "<<order.Quantity<<std::endl;
            }
        }

    }


    void cancelOrder(unsigned long long int id){
        unsigned long long int IdLookup=id;
        auto findId=idToOrder.find(IdLookup);
        if (findId == idToOrder.end()){
            std::cout<<"Zlecenie o id "<<id<<" nie istnieje"<<std::endl;
            return;
        }
        OrderLocation loc = findId->second;
        if (loc.isBuy) {
            bids[loc.price].erase(loc.it);

            if (bids[loc.price].empty()) {
                bids.erase(loc.price);
            }
            std::cout<<"Usunieto zlecenie kupna nr. "<<IdLookup<<std::endl;
        } else {
            asks[loc.price].erase(loc.it);
            if (asks[loc.price].empty()) {
                asks.erase(loc.price);
            }
            std::cout<<"Usunieto zlecenie sprzedaży nr. "<<IdLookup<<std::endl;
        }

        idToOrder.erase(IdLookup);

    }
    private:
        std::map<unsigned long long int, std::list<Order>> asks; //from smallest to biggest
        std::map<unsigned long long int, std::list<Order>, std::greater<unsigned long long int>> bids; //from biggest to smallest
        std::unordered_map<unsigned long long int, OrderLocation> idToOrder;
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
    std::unordered_map<std::string, std::unique_ptr<OrderBookThread>> shards;

public:
    void addOrder(Order ord) {
        if (shards.find(ord.symbol) == shards.end()) {
            shards[ord.symbol] = std::make_unique<OrderBookThread>();
            std::cout << "[Dispatcher] Tworzę nowy wątek dla symbolu: " << ord.symbol << std::endl;
        }

        shards[ord.symbol]->submitOrder(ord);
    }
};

int main() {
    Order bids[2] = {
        Order(1, 105, 10, true), //oferty kupna
        Order(2, 110, 5, true)
    };
    Order asks[2]={
        Order(1, 105, 10, false), //oferty sprzedazy
        Order(2, 110, 5, false)
    };
    OrderBook orderBook;

    orderBook.addOrder(bids[0]);
    orderBook.addOrder(bids[1]);
    orderBook.addOrder(asks[0]);
    orderBook.addOrder(asks[1]);
    orderBook.matchOrders();
    orderBook.printBook();
    orderBook.cancelOrder(1);
    return 0;
}