// StorageCache.h
#ifndef STORAGE_CACHE_H
#define STORAGE_CACHE_H

#include <unordered_map>
#include <mutex>
#include <vector>

struct Node {
    int key;
    int value;
    Node* prev;
    Node* next;
    Node(int k = 0, int v = 0) : key(k), value(v), prev(nullptr), next(nullptr) {}
};

class NodePool {
private:
    std::vector<Node> pool;
    Node* freeList;
    size_t capacity;
public:
    NodePool(size_t cap);
    Node* allocate(int key, int value);
    void deallocate(Node* node);
};

class StorageCache {
private:
    const int capacity;
    NodePool pool;
    std::unordered_map<int, Node*> cache;
    Node* head;     // MRU dummy head
    Node* tail;     // LRU dummy tail
    mutable std::mutex mutex;

    void remove(Node* node);
    void insertToFront(Node* node);
    void moveToFront(Node* node);
    Node* removeLRU();

public:
    explicit StorageCache(int cap);
    ~StorageCache();

    int get(int key);
    void put(int key, int value);
    void freeCache();
    void printState() const;
};

#endif