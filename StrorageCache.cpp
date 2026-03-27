// StorageCache.cpp
#include "StorageCache.h"
#include <stdexcept>

// ====================== NodePool ======================

NodePool::NodePool(size_t cap) : capacity(cap), freeList(nullptr) {
    if (cap == 0) return;
    pool.resize(cap);
    for (size_t i = 0; i < cap; ++i) {
        pool[i].next = freeList;
        freeList = &pool[i];
    }
}

Node* NodePool::allocate(int key, int value) {
    if (!freeList) throw std::bad_alloc();
    Node* node = freeList;
    freeList = freeList->next;
    node->key = key;
    node->value = value;
    node->prev = node->next = nullptr;
    return node;
}

void NodePool::deallocate(Node* node) {
    if (!node) return;
    node->next = freeList;
    freeList = node;
    node->prev = nullptr;
}

// ====================== StorageCache ======================

StorageCache::StorageCache(int cap)
    : capacity(cap > 0 ? cap : 0),
      pool(cap),
      head(nullptr),
      tail(nullptr) {
    if (capacity == 0) return;

    head = pool.allocate(0, 0);
    tail = pool.allocate(0, 0);
    head->next = tail;
    tail->prev = head;
}

StorageCache::~StorageCache() {
    std::lock_guard<std::mutex> lock(mutex);
    cache.clear();
}

void StorageCache::remove(Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = nullptr;
}

void StorageCache::insertToFront(Node* node) {
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
}

void StorageCache::moveToFront(Node* node) {
    remove(node);
    insertToFront(node);
}

StorageCache::Node* StorageCache::removeLRU() {
    if (head->next == tail) return nullptr;
    Node* lru = tail->prev;
    remove(lru);
    return lru;
}

int StorageCache::get(int key) {
    if (capacity == 0) return -1;

    std::lock_guard<std::mutex> lock(mutex);
    auto it = cache.find(key);
    if (it == cache.end()) return -1;

    Node* node = it->second;
    moveToFront(node);
    return node->value;
}

void StorageCache::put(int key, int value) {
    if (capacity == 0) return;

    std::lock_guard<std::mutex> lock(mutex);

    auto it = cache.find(key);
    if (it != cache.end()) {
        Node* node = it->second;
        node->value = value;
        moveToFront(node);
        return;
    }

    if (static_cast<int>(cache.size()) >= capacity) {
        Node* lru = removeLRU();
        if (lru) {
            cache.erase(lru->key);
            pool.deallocate(lru);
        }
    }

    Node* newNode = pool.allocate(key, value);
    insertToFront(newNode);
    cache[key] = newNode;
}

void StorageCache::freeCache() {
    std::lock_guard<std::mutex> lock(mutex);
    cache.clear();
}

void StorageCache::printState() const {
    std::lock_guard<std::mutex> lock(mutex);
    if (!head) {
        std::cout << "[Empty Cache]" << std::endl;
        return;
    }
    std::cout << "[MRU -> LRU] ";
    for (Node* curr = head->next; curr != tail; curr = curr->next) {
        std::cout << curr->key << " ";
    }
    std::cout << std::endl;
}