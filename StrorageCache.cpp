// StorageCache.cpp
#include "StorageCache.h"
#include <iostream>
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
    if (!freeList) return nullptr; // 풀 소진 시 안전하게 nullptr 반환

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
    pool(cap > 0 ? cap + 2 : 0),
    head(nullptr),
    tail(nullptr)
{
    if (capacity == 0) return;


    head = pool.allocate(0, 0);
    tail = pool.allocate(0, 0);

    head->next = tail;
    tail->prev = head;
}

StorageCache::~StorageCache()
{
    // 리스트에 연결된 모든 실제 Node를 Memory Pool로 반환
    if (head) {
        Node* curr = head->next;
        while (curr && curr != tail) {
            Node* nextNode = curr->next;
            pool.deallocate(curr);
            curr = nextNode;
        }

        // dummy head와 dummy tail도 pool로 반환
        pool.deallocate(head);
        pool.deallocate(tail);
    }

    // unordered_map 정리
    cache.clear();

    // 안전하게 포인터 초기화
    head = nullptr;
    tail = nullptr;
}

// 노드를 리스트에서 제거 (O(1))
void StorageCache::remove(Node* node) {
    if (!node) return;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = nullptr;
}

// 노드를 리스트의 가장 앞(MRU 위치)에 삽입 (O(1))
void StorageCache::insertToFront(Node* node) {
    if (!node) return;
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
}

// 노드를 MRU 위치로 이동 (LRU 정책의 핵심)
void StorageCache::moveToFront(Node* node) {
    if (!node) return;
    remove(node);
    insertToFront(node);
}

// 가장 오래된 노드(LRU) 제거 (O(1))
Node* StorageCache::removeLRU() {
    if (head->next == tail) return nullptr;
    Node* lru = tail->prev;
    remove(lru);
    return lru;
}

// ====================== get ======================
// get 연산이 O(1)인 이유:
// 1. unordered_map의 find() → 평균 O(1)
// 2. moveToFront() → Doubly Linked List에서 포인터만 조작 → O(1)
// 전체적으로 상수 시간에 동작
int StorageCache::get(int key) {
    if (capacity == 0) return -1;

    std::lock_guard<std::mutex> lock(mutex);

    auto it = cache.find(key);                    // unordered_map 조회 → O(1)
    if (it == cache.end()) return -1;

    Node* node = it->second;                      // map에서 Node 포인터 즉시 획득
    moveToFront(node);                            // 최근 사용된 것으로 갱신 (LRU 핵심)

    return node->value;
}

// ====================== put ======================
// put 연산이 O(1)인 이유:
// 1. unordered_map find() → O(1)
// 2. moveToFront() 또는 insertToFront() → O(1)
// 3. removeLRU() + erase() → O(1)
// 전체적으로 상수 시간에 동작
void StorageCache::put(int key, int value) {
    if (capacity == 0) return;

    std::lock_guard<std::mutex> lock(mutex);

    auto it = cache.find(key);                    // unordered_map 조회 → O(1)
    if (it != cache.end()) {
        Node* node = it->second;
        node->value = value;
        moveToFront(node);                        // MRU로 이동 → O(1)
        return;
    }

    // 용량 초과 시 LRU 제거
    if (static_cast<int>(cache.size()) >= capacity) {
        Node* lru = removeLRU();                  // LRU 노드 제거 → O(1)
        if (lru) {
            cache.erase(lru->key);                // map에서도 제거 → O(1)
            pool.deallocate(lru);
        }
    }

    Node* newNode = pool.allocate(key, value);
    if (newNode) {
        insertToFront(newNode);
        cache[key] = newNode;
    }
}

void StorageCache::freeCache() {
    std::lock_guard<std::mutex> lock(mutex);
    cache.clear();
}

void StorageCache::printState(int type, int key, int returnValue) {
    if (type == 0) {
        std::cout << "[put:" << key << "] 현재 최신순: ";
    }
    else {
        std::cout << "[get:" << key << "/반환값:" << returnValue << "] 현재 최신순: ";
    }

    if (!head || head->next == tail) {
        std::cout << "비어있음" << std::endl;
        return;
    }

    Node* curr = head->next;
    bool first = true;
    while (curr && curr != tail) {
        if (!first) std::cout << " -> ";
        std::cout << curr->key;
        first = false;
        curr = curr->next;
    }
    std::cout << std::endl;
}

StorageCache* createCache(int capacity) {
    return new StorageCache(capacity);
}

void put(StorageCache* store, int key, int value) {
    if (store) {
        store->put(key, value);
        store->printState(PUT, key, 0);
    }
}

int get(StorageCache* store, int key) {
    int value = -1;

    if (store) {
        value = store->get(key);
        store->printState(GET, key, value);
    }

    return value;
}

void freeCache(StorageCache* store) {
    if (store) {
        store->freeCache();
        delete store;
    }
}