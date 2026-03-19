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
#include <chrono>
#include <iomanip>
const size_t MAX_PRICE_LEVELS = 1000000;
const size_t MAX_ORDERS=1000000;
struct Order {
    unsigned long long int symbolId;
    unsigned long long int OrderId;
    unsigned long long int Price;
    unsigned long int Quantity;
    bool isBuy;
    bool isActive;
    unsigned long long int prevIndex = -1;
    unsigned long long int nextIndex = -1;
    Order() : OrderId(0), Price(0), Quantity(0), isBuy(false) {}
    Order(unsigned long long int id, unsigned long long int p, unsigned long int q, bool buy)
        : OrderId(id), Price(p), Quantity(q), isBuy(buy) {}

};

class OrderPool {
private:
    std::vector<Order> pool;
    uint32_t freeHead;

public:
    OrderPool(size_t capacity) {
        pool.resize(capacity);
        for (size_t i=0; i<capacity; i++) {
            pool[i].nextIndex=i+1;
        }
        pool[capacity-1].nextIndex=0;
        freeHead=0;
    }

    uint32_t allocate(){
        if (freeHead==(uint32_t)-1) [[unlikely]]{
            return -1;
        }
        uint32_t index=freeHead;
        freeHead=pool[index].nextIndex;
        pool[index].prevIndex=-1; //order closer to beginning, closer to head
        pool[index].nextIndex=-1; //order further from beginning, further from head
        pool[index].isActive=true;
        return index;
    }

    void free(uint32_t index) {
        pool[index].isActive=false;
        pool[index].nextIndex=freeHead;
        freeHead=index;
    }

    Order& get(uint32_t index) {
        return pool[index];
    }



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

struct PriceLevel {
    uint32_t headIndex=-1; // first order in queue(oldest)
    uint32_t lastIndex=-1; //last order in queue(newest)
    unsigned long long int totalVolume = 0;

};

class OrderBook {
private:
    OrderPool& orderPool;
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;

    std::vector<uint32_t> orderIdToIndex;
    public:
    OrderBook(OrderPool& pool) : orderPool(pool),  bids(MAX_PRICE_LEVELS), asks(MAX_PRICE_LEVELS) {
        orderIdToIndex.resize(MAX_ORDERS, -1);
    }
    void addOrder(unsigned long long int orderId, unsigned long long int price, unsigned long int quantity, bool isBuy) {
        uint32_t newIndex=orderPool.allocate();

        Order& order= orderPool.get(newIndex);
        order.OrderId=orderId;
        order.Price=price;
        order.Quantity=quantity;
        order.isBuy=isBuy;

        orderIdToIndex[orderId]=newIndex;

        PriceLevel& level=isBuy?bids[price]:asks[price];
        level.totalVolume+=quantity;

        if (level.headIndex==(uint32_t)-1) [[unlikely]]{
            level.headIndex=newIndex;
            level.lastIndex=newIndex;
        }
        else {
            order.prevIndex=level.lastIndex;
            orderPool.get(level.lastIndex).nextIndex=newIndex;
            level.lastIndex=newIndex;
        }

        if (isBuy && price > bestBid) {
            bestBid = price;
        } else if (!isBuy && price < bestAsk) {
            bestAsk = price;
        }

        matchOrders();

    }

    void matchOrders() {
        while (bestBid >= bestAsk && bestAsk < MAX_PRICE_LEVELS && bestBid > 0) {
            PriceLevel& askLevel=asks[bestAsk]; //i get price level of best ask and best bid (using price as index)
            PriceLevel& bidLevel=bids[bestBid];
            if (askLevel.headIndex == (uint32_t)-1) {
                bestAsk++;
                continue;
            }

            if (bidLevel.headIndex == (uint32_t)-1) {
                bestBid--;
                continue;
            }

            uint32_t askOrderIdx=askLevel.headIndex;
            uint32_t bidOrderIdx=bidLevel.headIndex;

            Order& sellOrder=orderPool.get(askOrderIdx);
            Order& buyOrder=orderPool.get(bidOrderIdx);

            unsigned long quantityToTrade = std::min(sellOrder.Quantity, buyOrder.Quantity);

            sellOrder.Quantity -= quantityToTrade;
            buyOrder.Quantity -= quantityToTrade;

            askLevel.totalVolume -= quantityToTrade;
            bidLevel.totalVolume -= quantityToTrade;

            if (sellOrder.Quantity==0) {
                askLevel.headIndex=sellOrder.nextIndex;
                if (askLevel.headIndex == (uint32_t)-1) {
                    orderPool.get(askLevel.headIndex).prevIndex=-1;
                }
                else {
                    askLevel.lastIndex=-1;
                }

                orderIdToIndex[sellOrder.OrderId]=-1;
                orderPool.free(askOrderIdx);
            }
            if (buyOrder.Quantity==0) {
                bidLevel.headIndex=buyOrder.nextIndex;

                if (bidLevel.headIndex != (uint32_t)-1) {
                    orderPool.get(bidLevel.headIndex).prevIndex=-1;
                }
                else {
                    bidLevel.lastIndex=-1;
                }
                orderIdToIndex[buyOrder.OrderId]=-1;
                orderPool.free(bidOrderIdx);
            }

        }
    }

    void printBook() {

        std::cout<<"ASKS"<<std::endl;
        for (size_t i=bestAsk; i<MAX_PRICE_LEVELS; i++ )
        {
            uint32_t currIndex=asks[i].headIndex;

            while (currIndex != (uint32_t)-1) {
                Order& order=orderPool.get(currIndex);
                if (order.isActive) {
                    std::cout << order.OrderId << " " << order.Price << " " << order.Quantity << std::endl;
                }
                currIndex=order.nextIndex;
            }
        }
        std::cout<<"BIDS"<<std::endl;
        for (size_t i=bestBid; i>0; i--)
        {
            uint32_t currIndex=bids[i].headIndex;

            while (currIndex != (uint32_t)-1) {
                Order& order=orderPool.get(currIndex);
                if (order.isActive) {
                    std::cout << order.OrderId << " " << order.Price << " " << order.Quantity << std::endl;
                }
                currIndex=order.nextIndex;
            }
        }

    }


    void cancelOrder(unsigned long long int orderId){
        if (orderId>=orderIdToIndex.size()) [[unlikely]]{
            return;
        }

        uint32_t orderIndex=orderIdToIndex[orderId];

        if (orderIndex==(uint32_t)-1) {
            return;
        }

        Order& order=orderPool.get(orderIndex);
        unsigned long long int price=order.Price;
        bool isBuy=order.isBuy;
        unsigned long int quantity=order.Quantity;

        PriceLevel& level=isBuy?bids[price]:asks[price];
        level.totalVolume-=quantity;

        uint32_t prev=order.prevIndex;
        uint32_t next=order.nextIndex;

        if (prev!=(uint32_t)-1) {
            orderPool.get(prev).nextIndex=next;
        }
        else {
            level.headIndex=next;
        }

        if (next!=(uint32_t)-1) {
            orderPool.get(next).prevIndex=prev;
        }
        else {
            level.lastIndex=prev;
        }

        orderIdToIndex[orderId] = -1;
        orderPool.free(orderIndex);   // Zwrócenie slotu do puli
    }
    private:
        //std::vector<std::vector<Order>> asks; //from smallest to biggest
        //std::vector<std::vector<Order>> bids; //from biggest to smallest
        //std::unordered_map<unsigned long long int, OrderLocation> idToOrder;
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

int main() {
    const int NUM_ORDERS = 1000000;
    ExchangeDispatcher dispatcher;

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

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    double ops = NUM_ORDERS / diff.count();

    std::cout << "--- WYNIKI BENCHMARKU ---" << std::endl;
    std::cout << "Czas calkowity: " << std::fixed << std::setprecision(4) << diff.count() << " s" << std::endl;
    std::cout << "Przepustowosc: " << std::fixed << std::setprecision(0) << ops << " zlecen/sekunda" << std::endl;

    return 0;
}
