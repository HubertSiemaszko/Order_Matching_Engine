# C++ High-Performance Order Matching Engine

I built this ultra-low-latency order matching engine from scratch in C++ to deepen my understanding of finance, stock market mechanics, and high-performance computing. Capable of processing millions of orders per second, this project serves as a practical exploration of hardware-sympathetic data structures, multithreading, and asynchronous systems. 

*Note: This project is currently a work-in-progress as I continuously refine the C++ mechanics to push execution speed, optimize cache-locality, and eliminate bottlenecks.*

## Technical Highlights

### Algorithmic & Memory Optimization

* **O(1) Price-Level Bucketing:** Designed a highly optimized Limit Order Book (LOB) that completely bypasses traditional tree-based (`std::map`) structures, which typically operate in O(log N) time. Instead, it utilizes indexed price-level buckets (`std::vector<std::vector<Order>>`) for true constant-time O(1) access to price points.
* **Hybrid Storage Architecture:** The engine intelligently splits data responsibilities. It uses contiguous memory arrays (`std::vector`) for rapid, cache-friendly order matching, paired with an `std::unordered_map` for instant O(1) direct order tracking, modifications, and cancellations.
* **Cache-Locality & Branch Prediction:** Data structures and matching loops are heavily optimized to keep memory access contiguous. This actively minimizes CPU cache misses and branch mispredictions during the critical path of the order matching loop.

### Multithreading & Concurrency

* **Symbol-Based Sharding:** Implemented an `ExchangeDispatcher` that partitions incoming order traffic by trading symbol (e.g., AAPL, TSLA). Each asset is routed to its own dedicated `OrderBookThread`, ensuring high throughput and isolated execution.
* **Asynchronous Processing Loop:** Offloads order book ingestion and matching to background worker threads. The system utilizes a non-blocking task submission model using `std::queue`, `std::mutex`, and `std::condition_variable` to guarantee the main dispatcher never blocks.
* **Deterministic Execution:** Thread safety is strictly managed to prevent race conditions while maintaining the strict chronological ordering required by financial exchanges.

### Financial Mechanics

* **Price-Time Priority:** Accurately simulates real-world exchange mechanics by matching crossing bids and asks based on strict price and time-of-arrival priority.
* **Dynamic BBO Tracking:** Continuously and efficiently tracks the Best Bid and Offer (BBO) to ensure the matching engine evaluates the spread instantly without unnecessary iterations.

## Tech Stack

* **Language:** C++
* **Concurrency:** `<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>`
* **Data Structures:** `<vector>`, `<unordered_map>`, `<deque>`, `<queue>`

## Build & Run

```bash
mkdir build && cd build
cmake ..
make
./main
