// SmartStorage.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "StorageCache.h"
#include <iostream>
#include <windows.h>

int main()
{
    // 1. 용량 4로 저장소 초기화
    StorageCache* store = createCache(4);

    // 2. 데이터 순차 저장
    put(store, 1, 100);
    // 현재 최신순: 1

    put(store, 2, 200);
    // 현재 최신순: 2 -> 1

    put(store, 3, 300);
    // 현재 최신순: 3 -> 2 -> 1

    put(store, 4, 400); // 저장소 가득 참
    // 현재 최신순: 4 -> 3 -> 2 -> 1 (가장 오른쪽 '1'이 삭제 후보)

    // 3. 데이터 조회 (우선순위 변경 발생)
    get(store, 1);      // 반환값: 100
    // 1번을 조회했으므로 가장 최신 위치로 이동합니다.
    // 현재 최신순: 1 -> 4 -> 3 -> 2 (이제 '2'가 삭제 후보)

    // 4. 용량 초과 데이터 저장 (Eviction 발생)
    put(store, 5, 500); // 새로운 Key 등장! 용량(4) 초과!
    // 가장 뒤에 있는(오래된) 2번을 제거하고, 5번을 저장합니다.
    // 현재 최신순: 5 -> 1 -> 4 -> 3

    // 5. 삭제된 데이터 확인
    get(store, 2);      // 반환값: -1 (찾을 수 없음)

    // 6. 연속 저장 및 조회
    put(store, 6, 600); // 용량 초과!
    // 가장 뒤에 있는 3번을 제거하고, 6번을 저장합니다.
    // 현재 최신순: 6 -> 5 -> 1 -> 4

    get(store, 3);      // 반환값: -1 (제거됨)
    get(store, 1);      // 반환값: 100 (아직 살아있음)
    // 현재 최신순: 1 -> 6 -> 5 -> 4
    get(store, 5);      // 반환값: 500
    // 현재 최신순: 5 -> 1 -> 6 -> 4

    // 7. 메모리 정리
    freeCache(store);
}