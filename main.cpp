#include <cstdint>
#include <deque>
#include <iostream>
#include <vector>
#include <map>
#include <list>
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

struct OrderLocation {
    unsigned long long int price;
    bool isBuy;
    std::list<Order>::iterator it;
};

class OrderBook {
    public:
    void addOrder(Order newOrder) {
        if (newOrder.isBuy) {
            bids[newOrder.Price].push_back(newOrder);
            idToOrder[newOrder.OrderId]=&asks[newOrder.Price].back();
            std::cout<<"Dodano zlecenie Kupna:"<<newOrder.OrderId<<std::endl;
            std::cout<<"Wartosc:"<<newOrder.Price<<std::endl;
            std::cout<<"Ilosc:"<<newOrder.Quantity<<std::endl;
        }
        else {
            asks[newOrder.Price].push_back(newOrder);
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
                itAsk->second.pop_front();
                if (itAsk->second.empty()) {
                    asks.erase(itAsk);
                }
            }

            if (buyOrder.Quantity == 0) {
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


    void IDLookup(){
        unsigned long long int IdLookup;
        std::cin>>IdLookup;
        auto findId=idToOrder.find(IdLookup);
        if (findId != idToOrder.end()){
            findId->second->
        }
    }
    private:
        std::map<unsigned long long int, std::deque<Order>> asks; //from smallest to biggest
        std::map<unsigned long long int, std::deque<Order>, std::greater<unsigned long long int>> bids; //from biggest to smallest
        std::unordered_map<unsigned long long int, Order*> idToOrder;
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
    return 0;
}