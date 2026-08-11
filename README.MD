# StorageCache - LRU Cache Implementation

## 1. 구현 이해 및 설계 목표

이 구현은 메모리 자원이 제한된 환경에서 **빈도 높은 get과 put 요청을** 효율적으로 처리하는 **LRU(Least Recently Used) 캐싱 모듈**입니다.

주요 요구사항은 다음과 같습니다:

- 고정된 capacity를 가진 저장소
- `get(key)`: key가 존재하면 value를 반환하고, 해당 데이터를 **최근 사용(MRU)** 으로 갱신
- `put(key, value)`: key가 존재하면 값을 overwrite하고 **MRU로 갱신**, 존재하지 않으면 **삽입** (용량 초과 시 **LRU eviction**)
- 모든 연산을 **평균 O(1)** 시간에 수행

본 구현은 위 요구사항을 만족하면서, 자료구조 설계 능력과 알고리즘 최적화를 중점으로 개발했습니다.

## 2. 자료구조 설계 및 선택 이유

LRU Cache를 O(1)으로 구현하기 위해 다음 두 가지 자료구조를 조합했습니다:

- **`std::unordered_map<int, Node*>`**
- **`Doubly Linked List`**

**unordered_map의 역할**

- key를 입력받았을 때 해당 Node를 **즉시(O(1))** 찾을 수 있게 해줍니다.
- `get()`과 `put()`에서 가장 먼저 수행되는 작업인 `key가 존재하는가?`를 확인합니다.
- 해시맵이 없으면 O(n) 선형 탐색이 발생합니다.

**Doubly Linked List의 역할**

- LRU의 핵심인 `가장 최근에 사용된 데이터를 맨 앞으로 이동`하는 작업을 O(1)로 가능하게 합니다.
- Doubly Linked List를 사용하여 **MRU 갱신을 위한 moveToFront 함수와 LRU 제거를 위한 removeLRU 함수를 구현**했습니다.

이 두 구조를 사용하여

- 조회/갱신은 unordered_map으로 O(1)
- 순서 변경은 doubly linked list로 O(1)
- 전체 연산이 평균 O(1)이 됩니다.

## 3. O(1) 시간 복잡도 상세 분석

### 3.1 get(int key) 연산이 O(1)인 이유

```cpp
int StorageCache::get(int key) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = cache.find(key);           // unordered_map 조회 -> O(1)
    if (it == cache.end()) return -1;

    Node* node = it->second;             // map에서 Node 포인터 획득
    moveToFront(node);                   // MRU로 이동 -> O(1)
    return node->value;
}
```

- `cache.find(key)`: unordered_map의 평균 O(1) 해시 조회
- `moveToFront(node)`: Doubly Linked List에서 노드를 제거하고 맨 앞으로 삽입하므로 포인터만 조작하여 O(1)
- 전체 get 연산: 평균 O(1)

### 3.2 put(int key, int value) 연산이 O(1)인 이유

```cpp
void StorageCache::put(int key, int value) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = cache.find(key);                    // unordered_map 조회 -> O(1)
    if (it != cache.end()) {
        Node* node = it->second;
        node->value = value;                      // 값 갱신
        moveToFront(node);                        // MRU로 이동 -> O(1)
        return;
    }

    if (static_cast<int>(cache.size()) >= capacity) {
        Node* lru = removeLRU();                  // LRU 제거 -> O(1)
        if (lru) {
            cache.erase(lru->key);                // map에서 제거 -> O(1)
            delete lru;
        }
    }

    Node* newNode = new Node(key, value);         // 새 노드 생성
    insertToFront(newNode);                       // MRU 위치에 삽입 -> O(1)
    cache[key] = newNode;                         // map에 등록 -> O(1)
}
```

- `cache.find(key)` -> O(1)
- `moveToFront()` 또는 `insertToFront()` -> Doubly Linked List 포인터 조작으로 O(1)
- `removeLRU()` -> `tail->prev`를 제거하므로 O(1)
- `cache.erase()` -> unordered_map 삭제로 평균 O(1)
- 전체 put 연산: 평균 O(1)

## 4. 요구사항 기반 기능 개선

메모리 자원이 제한된 환경과 매우 빈번한 데이터 조회/쓰기 요청을 고려하여, 기본 구현 이후 두 가지 주요 개선을 단계적으로 적용했습니다.

### 4.1 Fixed-size Memory Pool 도입

메모리 제한 환경을 반영하여 Node의 할당 방식을 개선했습니다.

초기에는 모든 Node를 `new`와 `delete`로 직접 관리했습니다. 그러나 고빈도 요청이 발생하는 LRU 캐시에서는 Node의 생성과 삭제가 매우 빈번하게 일어나, 할당 오버헤드와 메모리 단편화가 발생할 가능성이 있었습니다.

이를 해결하기 위해 Fixed-size Memory Pool을 도입했습니다.

- capacity만큼 Node를 미리 할당
- `freeList`를 통해 사용이 끝난 Node를 재사용
- `new`/`delete` 호출을 최소화하고 메모리 단편화를 방지
- 메모리 사용량을 예측 가능하게 관리

이 개선으로 메모리 자원이 제한된 환경에서도 더 안정적이고 효율적인 동작을 기대할 수 있게 되었습니다.

### 4.2 Dummy Head와 Dummy Tail 도입

Doubly Linked List 초기 버전에서는 head와 tail이 nullptr인지 확인하는 로직이 여러 함수에 들어갔습니다.

- insertToFront() - 첫 노드 삽입 시 특별 처리

```cpp
if (head == nullptr) {
    head = node;
    tail = node;
    node->prev = node->next = nullptr;
    return;
}
```

- remove() - head 또는 tail일 때 특별 처리

```cpp
if (node == head) head = node->next;
if (node == tail) tail = node->prev;
```

- moveToFront() - head일 때 체크

```cpp
if (!node || node == head) return;
```

- removeLRU() - tail이 nullptr일 때

```cpp
if (tail == nullptr) return nullptr;
```

- printState() - head가 nullptr일 때

```cpp
if (!head) {
    std::cout << "[Empty]" << std::endl;
    return;
}
```

이런 분기 처리들이 여러 함수에 반복적으로 들어가면 코드 유지보수가 어려워질 수 있습니다. 그래서 Dummy Head와 Dummy Tail을 도입했습니다.

Dummy를 추가한 후에는 head와 tail이 항상 존재하게 되어, 리스트가 비어있을 때부터 가득 찰 때까지 불필요한 nullptr 체크 코드를 줄일 수 있었습니다. 결과적으로 코드가 더 간결해졌고, 경계 조건으로 인한 실수 가능성도 줄였습니다.

### 4.3 대용량 시나리오 대응

현재 구현은 고정된 capacity 환경에 최적화되어 있습니다.

만약 실제 시스템에서 capacity를 수십만에서 수백만 정도로 크게 설정해야 한다면, Memory Pool의 크기를 동적으로 조정하거나 필요에 따라 `new`/`delete` 기반 fallback 방식을 추가하는 하이브리드 구조를 고려할 수 있습니다.

## 5. 코드 품질 및 가독성

- 메서드를 작은 단위로 분리하여 기능별로 세분화했습니다.
- RAII 원칙을 준수하여 객체가 생성될 때 자원을 획득하고, 소멸될 때 해제되도록 설계했습니다.
- 경계 조건 처리를 최소화하여 전체 코드의 가독성과 유지보수성을 높였습니다.
- `std::mutex`를 활용해 get과 put 호출 시 동기화 처리를 했습니다.

## 6. 결론

본 StorageCache는 기본 자료구조 설계에서 시작하여, Fixed-size Memory Pool 도입을 통한 메모리 효율성 개선과 Dummy Head / Tail 도입을 통한 코드 일관성 및 안정성 개선을 단계적으로 반영했습니다.

이 과정에서 요구사항에 맞는 자료구조 설계, 알고리즘 최적화, 코드 품질, 구현 이해도를 종합적으로 담고자 했습니다.
