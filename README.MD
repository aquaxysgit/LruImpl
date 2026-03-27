# StorageCache - LRU Cache Implementation

## 1. 과제 이해 및 설계 목표

이번 과제는 **메모리 자원이 제한된 환경**에서 고빈도 `get`과 `put` 요청을 효율적으로 처리하는 LRU(Least Recently Used) 기반 스마트 캐싱 모듈을 구현하는 것입니다.

주요 요구사항은 다음과 같습니다:
- 고정된 용량(capacity)을 가진 저장소
- `get(key)`: 데이터 존재 시 value 반환 + 해당 데이터를 **최근 사용(MRU)** 으로 갱신
- `put(key, value)`: 키 존재 시 overwrite + MRU 갱신, 용량 초과 시 **가장 오래된(LRU)** 데이터 제거
- 모든 연산을 **평균 O(1)** 시간에 수행

본 구현은 위 요구사항을 정확히 만족하면서, 코드의 가독성, 안정성, 그리고 실무적 고려사항을 균형 있게 반영했습니다.

## 2. 자료구조 설계

**선택한 구조**: `std::unordered_map` + Doubly Linked List (직접 구현)

- `unordered_map<int, Node*>`: key를 통해 Node에 O(1) 접근
- Doubly Linked List: 노드의 삽입, 삭제, 순서 재배치를 O(1)로 처리

이 조합은 LRU Cache를 O(1)으로 구현하는 가장 표준적이고 효과적인 방법입니다.

### Dummy Head와 Dummy Tail 도입

Doubly Linked List를 직접 구현할 때 가장 까다로운 부분은 **경계 조건(Edge Case)** 입니다.

리스트가 처음 비어있을 때, 노드가 1개 또는 2개밖에 없을 때, 또는 eviction 후 노드가 적어졌을 때 `head`나 `tail`이 `nullptr`이 될 수 있습니다.  
이 경우 `remove()`, `moveToFront()`, `removeLRU()` 함수마다 `if (node == head)`, `if (node == tail)`, `if (node->prev == nullptr)` 같은 분기 처리가 반복적으로 필요해집니다.

이런 분기문이 많아지면 코드가 복잡해지고, 로직의 일관성이 깨지며 버그 발생 가능성이 높아집니다.

**Dummy Head와 Dummy Tail**을 도입하여 이 문제를 해결했습니다.

- `head`와 `tail`이 항상 존재하기 때문에, 리스트가 비어있을 때부터 가득 찰 때까지 **모든 연산에서 동일한 로직**을 사용할 수 있습니다.
- null 체크와 특별한 경계 처리 코드가 대폭 줄어들어 전체 코드가 더 간결하고 안전해졌습니다.
- 특히 과제 예시처럼 `put`과 `get`이 연속적으로 발생하며 순서가 자주 변경되는 상황에서 안정성이 크게 향상됩니다.

Dummy Head/Tail은 LRU 구현에서 널리 사용되는 실무 기법으로, 가독성과 안정성을 높이는 데 효과적입니다.

## 3. Memory Pool (선택적 적용)

메모리 제한 환경을 고려하여 Fixed-size Memory Pool을 적용했습니다.  
capacity만큼 Node를 미리 할당하고 `freeList`를 통해 재사용함으로써 `new`/`delete` 오버헤드와 메모리 단편화를 최소화했습니다.

다만 Memory Pool은 필수가 아닌 선택적 요소로, 간단한 `new`/`delete` 버전으로도 충분히 대체 가능합니다.  
본 구현에서는 과제의 메모리 제한 환경이라는 배경을 반영하기 위해 포함하였습니다.

## 4. 알고리즘 최적화

- `get(key)`: hashmap 조회 후 `moveToFront()` 호출 → O(1)
- `put(key, value)`: 키 존재 시 업데이트 + MRU 이동 / 키 없음 시 LRU eviction 후 삽입 → O(1)

## 5. 코드 품질 및 가독성

- 메서드를 작은 단위로 분리하여 책임을 명확히 함
- RAII 원칙 준수 (소멸자에서 자원 정리)
- `std::mutex`를 사용하여 기본적인 thread-safety 확보
- 경계 조건 처리를 최소화하여 가독성과 유지보수성 향상

## 6. 확장 가능성

- LFU Cache로의 확장
- Size-based eviction (데이터 실제 크기 기준)
- Sharded LRU (높은 동시성 환경 대응)

## 7. 제출 구성

- `StorageCache.h`
- `StorageCache.cpp`
- `README.md` (본 문서)
- `main.cpp` (과제 예시 검증용)



# StorageCache - LRU Cache Implementation

## 1. 과제 이해 및 설계 목표
본 과제는 **메모리 자원이 제한된 환경**에서 고빈도 `get`/`put` 요청을 효율적으로 처리하는 스마트 캐싱 모듈을 구현하는 것입니다.  
핵심 요구사항은 **LRU(Least Recently Used)** 정책을 정확히 준수하면서, 모든 연산을 **평균 O(1)** 시간에 수행하는 것입니다.

## 2. 자료구조 설계 (자료구조 설계 능력)

**선택한 구조**: `std::unordered_map` + `Doubly Linked List` (직접 구현)

- `unordered_map<int, Node*>`: key를 통해 Node에 O(1) 접근
- `Doubly Linked List`: 노드의 삽입, 삭제, 순서 변경을 O(1)에 수행
- **Dummy Head / Tail**: edge case (빈 리스트, 첫 번째/마지막 노드)에서 null 체크를 최소화하여 코드 가독성과 안정성 향상

**왜 이 조합인가?**
- Singly Linked List만 사용하면 중간 삭제 시 O(n)이 되어 O(1) 요구사항을 만족할 수 없음
- `std::list`를 사용하지 않은 이유: 과제에서 "기본 자료구조를 직접 조합"하라는 의도를 존중
- `unordered_map`의 worst-case O(n)은 실무 정상 데이터 분포에서는 거의 발생하지 않음

**추가 설계**: Fixed-size Memory Pool
- 메모리 제한 환경에서 `new`/`delete` 오버헤드와 단편화를 최소화
- capacity만큼 미리 할당하여 메모리 사용량을 예측 가능하게 함

## 3. 알고리즘 최적화 (시간 복잡도)

- `get(key)`: O(1) 평균  
  - 해시맵 조회 → 노드 존재 시 `moveToFront()` 호출
- `put(key, value)`: O(1) 평균  
  - 키 존재 시 업데이트 + `moveToFront()`
  - 키 없음 + 용량 초과 시 `removeLRU()` 후 삽입

모든 핵심 연산(링크 조작, 해시맵 접근)이 O(1)이므로 요구사항을 만족합니다.

## 4. 코드 품질 및 가독성

- **클래스 분리**: `StorageCache`, `NodePool`을 별도 클래스로 분리하여 책임 분리
- **메서드 분리**: `remove`, `insertToFront`, `moveToFront`, `removeLRU` 등 내부 로직을 명확히 분리
- **RAII 준수**: 소멸자에서 자원 정리
- **Thread Safety**: `std::mutex`를 사용하여 multi-thread 환경에서도 안전하게 동작
- **Defensive Programming**: capacity ≤ 0 처리, nullptr 체크 등

## 5. 구현 상세 설명

### 주요 흐름
- **get(key)**: 해시맵으로 Node 찾기 → `moveToFront()`로 MRU로 이동 → value 반환
- **put(key, value)**:
  - 키 존재 → 값 업데이트 + MRU 이동
  - 키 없음 → 용량 초과 시 LRU 제거 → 새 Node를 MRU 위치에 삽입

### Memory Pool 동작 원리
- 생성 시 capacity만큼 Node를 미리 할당
- `allocate()`: freeList에서 Node를 가져와 사용
- `deallocate()`: 사용 끝난 Node를 freeList로 반환

## 6. Edge Cases 처리
- capacity = 0 또는 음수 → 안전하게 처리
- 동일 key 반복 put → overwrite + MRU 이동
- eviction 후 삭제된 key 조회 → -1 반환
- capacity = 1 극한 상황

## 7. 확장 가능성 (실무 관점)
- LFU Cache로 확장 가능 (frequency counter 추가)
- Size-based eviction (value의 실제 크기 고려)
- Sharded LRU (mutex contention 감소를 위한 분할)
- TTL 지원 (Node에 expiry 시간 추가)

## 8. 빌드 및 테스트
```bash
g++ -std=c++11 -pthread -O2 StorageCache.cpp main.cpp -o lru_cache