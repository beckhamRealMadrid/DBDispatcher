# 🛠️ DBDispatcher System

![C++](https://img.shields.io/badge/C%2B%2B-High%20Performance-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)

---

## 🚩 **프로젝트 개요**

`DBDispatcher`는 **대규모 온라인 게임 서버의 DB 요청 병목 문제를 해결**하기 위해 설계된 고성능 DB 작업 디스패처 시스템입니다.  
- 유저 단위 직렬화 보장 ✅
- 전체 병렬 처리 ✅
- 고속 메모리 풀 ✅

---

## 🧱 **구성 요소**

### 1️⃣ `CDBDispatcher`
- **역할:** 전체 시스템의 허브. 유저별 직렬화/비직렬화 DB 요청을 관리하고 워커 스레드로 분배합니다.
- **핵심 메서드:**
  - `Open()`: 시스템 초기화 및 워커 스레드 생성
  - `QueueUserRequest()`: 유저 단위 직렬화 요청 등록
  - `QueueSharedRequest()`: 비직렬화 요청 등록
  - `Close()`: 안전 종료

---

### 2️⃣ `CUserDBQueue`
- **역할:** 각 유저별로 독립된 요청 큐를 관리하며 직렬화 처리를 보장합니다.
- **특징:** 원형 버퍼 구조로 고속 처리.

---

### 3️⃣ `CDBMsgPool` + `CDBMsg`
- **역할:** DB 요청 메시지를 메모리 풀로 관리, 재사용 최적화.
- **특징:**  
  - `move semantics(이동 연산 최적화)` 적용  
  - 최대 10,000개 요청 풀 지원

---

### 4️⃣ `CSemaphoreChannel`
- **역할:** 워커 스레드 간의 동기화 큐 (세마포어 기반).
- **특징:** 대기 중 스레드 즉시 깨우기 / 동시 처리 보장.

---

### 5️⃣ `CMssqlConnection`
- **역할:** ODBC 기반 SQL 커넥션 래퍼.
- **기능:** `Connect()`, `Disconnect()`, `Execute()`, `Query()`
- **특징:** RAII + std::vector<std::map<std::string, std::string>> 형태의 결과 반환.

---

## ⚙️ **구현 특징**

- ✅ **워크 스레드 풀**: CPU 코어 수 기반으로 4~8개의 워커 스레드 자동 할당
- ✅ **메모리 최적화**: `CDBMsg` 객체 이동 최적화 적용
- ✅ **풀 시스템**: 고정 크기 메모리 풀 기반, 런타임 중 동적 할당 없음
- ✅ **유저 직렬화 보장**: `CUserDBQueue`의 단일 락 기반 처리

---

## 📂 **디렉토리 구조**

```

/ProjectRoot
├── include/
│   ├── CDBDispatcher.h
│   ├── CUserDBQueue.h
│   ├── CDBMsg.h
│   ├── CDBMsgPool.h
│   ├── CSemaphoreChannel.h
│   ├── CMssqlConnection.h
│   └── ...
├── src/
│   ├── CDBDispatcher.cpp
│   ├── CUserDBQueue.cpp
│   ├── CDBMsg.cpp
│   ├── CDBMsgPool.cpp
│   ├── CSemaphoreChannel.cpp
│   ├── CMssqlConnection.cpp
│   └── ...
└── README.md

````

---

## 🧪 **테스트 코드 예시**

```cpp
CDBDispatcher aDispatcher;
aDispatcher.Open();

aDispatcher.QueueUserRequest(1001, 101, "UserPayload", strlen("UserPayload"));
aDispatcher.QueueSharedRequest(201, "SharedPayload", strlen("SharedPayload"));

std::this_thread::sleep_for(std::chrono::seconds(3));
aDispatcher.Close();
````

---

## 🛡️ **주의사항**

* **ODBC 설정 필수**: `CMssqlConnection`은 ODBC Driver 17 이상 필요.
* **UNICODE 환경 지원**: 유니코드 프로젝트의 경우 문자열 타입 유의.
* **풀 용량 조정**: `DB_REQUEST_POOL_CAPACITY`, `ON_MAX_CONNECTION` 상수로 설정 가능.

---

## 📜 **License**

MIT License © 2025

---
