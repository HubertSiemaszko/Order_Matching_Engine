#include <cstdint>
#include <deque>
#include <iostream>
#include <vector>
#include <map>
#include <functional>
struct Order {
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

class OrderBook {
    public:
    void addOrder(Order newOrder) {
        if (newOrder.isBuy) {
            bids[newOrder.Price].push_back(newOrder);
            std::cout<<"Dodano zlecenie Kupna:"<<newOrder.OrderId<<std::endl;
            std::cout<<"Wartosc:"<<newOrder.Price<<std::endl;
        }
        else {
            asks[newOrder.Price].push_back(newOrder);
            std::cout<<"Dodano zlecenie Sprzedaży:"<<newOrder.OrderId<<std::endl;
            std::cout<<"Wartosc:"<<newOrder.Price<<std::endl;
        }
        matchOrders();
    }

    void matchOrders(){
        if (asks.empty()||bids.empty()) {return;}
        unsigned long long int bestPriceAsk = asks.begin()->first;
        unsigned long long int bestPriceBid = bids.begin()->first;
        while (bestPriceBid >= bestPriceAsk) {
            bestPriceAsk = asks.begin()->first;
            bestPriceBid = bids.begin()->first;
            Order& sellOrder = asks.begin()->second.front();
            Order& buyOrder = bids.begin()->second.front();
            if (buyOrder.Quantity>=sellOrder.Quantity) {
                buyOrder.Quantity -= sellOrder.Quantity;
                asks.begin()->second.pop_front();
                if (asks.begin()->second.empty()) {
                    asks.erase(asks.begin());
                }

                if (buyOrder.Quantity==0) {
                    bids.begin()->second.pop_front();
                    if (asks.begin()->second.empty()) {
                        asks.erase(asks.begin());
                    }
                }
                std::cout<<"ZLECENIE WYKONANE pierwsza ZA "<<bestPriceAsk<<std::endl;
            }
            else if (buyOrder.Quantity<sellOrder.Quantity) {
                sellOrder.Quantity -= buyOrder.Quantity;
                bids.begin()->second.pop_front();
                if (asks.begin()->second.empty()) {
                    asks.erase(asks.begin());
                }
                std::cout<<"ZLECENIE WYKONANE druga ZA "<<bestPriceAsk<<std::endl;
            }
            else {
                break;
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

    }

    private:
        std::map<unsigned long long int, std::deque<Order>> asks; //from smallest to biggest
        std::map<unsigned long long int, std::deque<Order>, std::greater<unsigned long long int>> bids; //from biggest to smallest

};

int main() {
    Order bids[2] = {
        Order(1, 105, 10, true),
        Order(2, 110, 5, true)
    };
    Order asks[2]={
        Order(1, 105, 10, false),
        Order(2, 110, 5, false)
    };
    OrderBook orderBook;

    orderBook.addOrder(bids[0]);
    orderBook.addOrder(bids[1]);
    orderBook.addOrder(asks[0]);
    orderBook.addOrder(asks[1]);
    orderBook.matchOrders();
    orderBook.printBook();
    return 0;
}