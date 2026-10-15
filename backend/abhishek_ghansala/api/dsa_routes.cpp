#include "dsa_routes.h"

void registerDSARoutes(crow::App<crow::CORSHandler>& app,
                       ChargingQueue& chargingQueue,
                       SwapQueue& swapQueue,
                       TransactionStack& txnStack) {

    // GET /api/charging/queue - Visual FIFO Charging Queue state
    CROW_ROUTE(app, "/api/charging/queue").methods(crow::HTTPMethod::GET)
    ([&chargingQueue](){
        std::string items[CHARGING_QUEUE_CAPACITY];
        int count = chargingQueue.getItems(items, CHARGING_QUEUE_CAPACITY);

        crow::json::wvalue res;
        res["frontIndex"] = chargingQueue.getFrontIndex();
        res["rearIndex"] = chargingQueue.getRearIndex();
        res["size"] = chargingQueue.getSize();
        res["capacity"] = CHARGING_QUEUE_CAPACITY;

        for (int i = 0; i < count; i++) {
            res["items"][i] = items[i];
        }

        return crow::response(200, res);
    });

    // GET /api/swap/queue - Visual FIFO Swap Queue state
    CROW_ROUTE(app, "/api/swap/queue").methods(crow::HTTPMethod::GET)
    ([&swapQueue](){
        std::string items[SWAP_QUEUE_CAPACITY];
        int count = swapQueue.getItems(items, SWAP_QUEUE_CAPACITY);

        crow::json::wvalue res;
        res["frontIndex"] = swapQueue.getFrontIndex();
        res["rearIndex"] = swapQueue.getRearIndex();
        res["size"] = swapQueue.getSize();
        res["capacity"] = SWAP_QUEUE_CAPACITY;

        for (int i = 0; i < count; i++) {
            res["items"][i] = items[i];
        }

        return crow::response(200, res);
    });

    // GET /api/transactions/stack - Visual LIFO Transaction Stack state
    CROW_ROUTE(app, "/api/transactions/stack").methods(crow::HTTPMethod::GET)
    ([&txnStack](){
        std::string items[TRANSACTION_STACK_CAPACITY];
        int count = txnStack.getItems(items, TRANSACTION_STACK_CAPACITY);

        crow::json::wvalue res;
        res["topIndex"] = txnStack.getTopIndex();
        res["size"] = txnStack.getSize();
        res["capacity"] = TRANSACTION_STACK_CAPACITY;

        for (int i = 0; i < count; i++) {
            res["items"][i] = items[i];
        }

        return crow::response(200, res);
    });
}
