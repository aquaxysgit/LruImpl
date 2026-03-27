int main() {
    StorageCache* store = new StorageCache(4);

    store->put(1, 100);
    store->put(2, 200);
    store->put(3, 300);
    store->put(4, 400);

    store->printState();

    std::cout << store->get(1) << std::endl;
    store->printState();

    store->put(5, 500);
    store->printState();

    std::cout << store->get(2) << std::endl;

    store->put(6, 600);
    store->printState();

    std::cout << store->get(3) << std::endl;
    std::cout << store->get(1) << std::endl;
    std::cout << store->get(5) << std::endl;

    store->freeCache();
    delete store;

    return 0;
}