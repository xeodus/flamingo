# Order Book Implementation

This is an order book implementation for low latency trading environments, written in C++. The order book supports multiple order type and ensures optimal trade execution.


**🔨 Project under development..**

- **Good For Day:** Pruned at market close.
- **Market Orders:** Converted to good till cancel if executable.
- **Limit Orders:** Match based on price-time priority.
- **Fill Or Kill:** Executed entirely or canceled.
- **Fill And Kill:** Executed partially and the remaining quantity canceled.

## 💡 Features 

- [x] **Efficient Matching Engine:** Implements price-time priority for optimal order execution.
- [x] **Concurrency Support:** Uses Mutex locks for thread safety.
- [x] **Auto-Pruning:** Autometically removes expired orders.
- [x] **Order Modification & Cancellations:** Allows real-time order updates.

## 🛡️ Thread Safety

The order book runs a background thread for GFD order pruning and uses mutex locks to ensure data consistency in a multi-threaded environment.

## Installation

```bash
    git clone https://github.com/xeodus/flamingo.git
    cd flamingo
