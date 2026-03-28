# StorageCache - LRU Cache Implementation

## 1. 과제 이해 및 설계 목표

이번 과제는 메모리 자원이 제한된 환경에서 **고빈도 get과 put 요청**을 효율적으로 처리하는 LRU(Least Recently Used) 캐싱 모듈을 구현하는 것입니다.

주요 요구사항은 다음과 같습니다:
- 고정된 capacity를 가진 저장소
- `get(key)`: key가 존재하면 value를 반환하고, 해당 데이터를 **최근 사용(MRU)** 으로 갱신
- `put(key, value)`: key가 존재하면 값을 overwrite하고 MRU로 갱신, 존재하지 않으면 삽입 (용량 초과 시 LRU eviction)
- 모든 연산을 **평균 O(1)** 시간에 수행

본 구현은 위 요구사항을 정확히 만족하면서, 자료구조 설계 능력과 알고리즘 최적화를 중점으로 개발했습니다.

## 2. 자료구조 설계 및 선택 이유

LRU Cache를 O(1)으로 구현하기 위해 다음 두 가지 자료구조를 조합했습니다:

- **`std::unordered_map<int, Node*>`**
- **Doubly Linked List**

**unordered_map의 역할**  
- key를 입력받았을 때 해당 Node를 **즉시(O(1))** 찾을 수 있게 해줍니다.
- `get()`과 `put()`에서 가장 먼저 수행되는 작업 : “key가 존재하는가?”를 확인합니다.
- 해시맵이 없으면 O(n) 선형 탐색이 발생합니다.

**Doubly Linked List의 역할**  
- LRU의 핵심인 “가장 최근에 사용된 데이터를 맨 앞으로 이동”하는 작업을 O(1)로 가능하게 합니다.
- Doubly Linked List를 사용하여 **LRU를 구현하기 위해 맨 앞으로 옮기는(moveToFront) 함수와 LRU eviction(removeLRU) 함수를 추가**하였습니다.

이 두 구조를 사용하여
- 조회/갱신은 unordered_map으로 O(1)
- 순서 변경은 doubly linked list로 O(1)
: 전체 연산이 평균 O(1)이 됩니다.

## 3. O(1) 시간 복잡도 상세 분석 (put, get 호출 설명)

### 3.1 get(int key) 연산이 O(1)인 이유

```
int StorageCache::get(int key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = cache.find(key);           // ① unordered_map 조회 → O(1)
    if (it == cache.end()) return -1;

    Node* node = it->second;             // ② map에서 Node 포인터 즉시 획득
    moveToFront(node);                   // ③ Doubly Linked List에서 위치 이동 → O(1)
    return node->value;
}
```
- cache.find(key): unordered_map의 평균 O(1) 해시 조회
- moveToFront(node): Doubly Linked List에서 노드를 제거하고 맨 앞으로 삽입 → 포인터만 조작하므로 O(1)
: 전체 get 연산 : 평균 O(1) 입니다.

### 3.2 put(int key, int value) 연산이 O(1)인 이유
```
void StorageCache::put(int key, int value) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = cache.find(key);                    // ① unordered_map 조회 → O(1)
    if (it != cache.end()) {
        Node* node = it->second;
        node->value = value;                      // 값 갱신
        moveToFront(node);                        // MRU로 이동 → O(1)
        return;
    }

    if (static_cast<int>(cache.size()) >= capacity) {
        Node* lru = removeLRU();                  // LRU 제거 → O(1)
        if (lru) {
            cache.erase(lru->key);                // map에서 제거 → O(1)
            delete lru;
        }
    }

    Node* newNode = new Node(key, value);         // 새 노드 생성
    insertToFront(newNode);                       // MRU 위치에 삽입 → O(1)
    cache[key] = newNode;                         // map에 등록 → O(1)
}
```
- cache.find(key) → O(1)
- moveToFront() 또는 insertToFront() → Doubly Linked List 포인터 조작 → O(1)
- removeLRU() → tail->prev를 제거 → O(1)
- cache.erase() → unordered_map 삭제 → O(1)
: 전체 put 연산 : 평균 O(1) 입니다.
