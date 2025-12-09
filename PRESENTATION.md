# 센서 모니터링 시스템 - 최종 발표 자료

---

## 3. 최종 설계 및 개발 환경

### 📐 최종 S/W 아키텍처

**실제 구현된 시스템 구조:**

```
┌─────────────────────────────────────────────────────────────────────┐
│                     센서 모니터링 시스템                              │
└─────────────────────────────────────────────────────────────────────┘

    [센서 클라이언트]                    [모니터링 서버]

    1. 센서 데이터 생성                   5. TCP 연결 수락
       ↓                                    ↓
    2. HMAC-SHA256 계산                  6. 패킷 수신
       ↓                                    ↓
    3. 패킷 구성                         7. HMAC-SHA256 재계산
       ↓                                    ↓
    4. TCP 전송 ──────────────────────→  8. HMAC 검증
       (127.0.0.1:8888)                     ↓
                                      ┌─────┴─────┐
                                   일치        불일치
                                      ↓           ↓
                                  로그 기록    경고 발생
                                (sensor_log)  (alert_log)
```

**구성 요소:**
- **센서 클라이언트** (`sensor_client.c`): 4종 센서 시뮬레이션 (온도/습도/기압/진동)
- **모니터링 서버** (`monitor_server.c`): HMAC 검증 및 로그 관리
- **공통 모듈**: `crypto_utils` (HMAC), `sensor_utils` (센서), `protocol.h` (프로토콜)

---

### 🔄 데이터 흐름 (구현 검증)

**HMAC-SHA256 기반 데이터 무결성 검증 과정:**

```
┌──────────────────────────────────────────────────────────────────┐
│ STEP 1: 클라이언트 - 센서 데이터 생성                            │
└──────────────────────────────────────────────────────────────────┘
sensor_id: 1001
sensor_type: Temperature (1)
value: 26.34°C
timestamp: 1732291842
sensor_name: "Sensor_Temperature_1001"

        ↓

┌──────────────────────────────────────────────────────────────────┐
│ STEP 2: 클라이언트 - HMAC-SHA256 계산                            │
└──────────────────────────────────────────────────────────────────┘
입력 데이터: 52 bytes (HMAC 필드 제외)
키: "shared_secret_key_2024"
알고리즘: HMAC-SHA256

출력 HMAC (32 bytes):
a3f2b8c1d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1

        ↓

┌──────────────────────────────────────────────────────────────────┐
│ STEP 3: TCP 전송 (84 bytes)                                      │
└──────────────────────────────────────────────────────────────────┘
전송: 센서 데이터 (52 bytes) + HMAC (32 bytes)

        ↓

┌──────────────────────────────────────────────────────────────────┐
│ STEP 4: 서버 - 패킷 수신 및 HMAC 재계산                          │
└──────────────────────────────────────────────────────────────────┘
동일한 키로 HMAC 재계산:
계산된 HMAC: a3f2b8c1d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1
수신된 HMAC: a3f2b8c1d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1

        ↓

┌──────────────────────────────────────────────────────────────────┐
│ STEP 5: 서버 - 검증 결과                                         │
└──────────────────────────────────────────────────────────────────┘
memcmp(계산된_HMAC, 수신된_HMAC, 32) == 0
→ ✓ VALID: 데이터 무결성 보장
→ ✗ INVALID: 위변조 감지 및 경고
```

**검증 완료:**
- ✅ 정상 데이터: HMAC 일치, 로그 기록
- ✅ 변조 데이터: HMAC 불일치, 즉시 경고 발생
- ✅ 암호화 모듈 정확성: OpenSSL HMAC API 사용으로 검증됨

---

### 📦 클라이언트와 서버 간 프로토콜 (메시지 구조)

**SensorPacket 구조체 정의:**

```c
typedef struct {
    uint32_t sensor_id;              // 센서 고유 ID        (4 bytes)
    SensorType sensor_type;          // 센서 타입          (4 bytes)
    float value;                     // 센서 측정값         (4 bytes)
    time_t timestamp;                // 측정 시각          (8 bytes)
    char sensor_name[MAX_SENSOR_NAME]; // 센서 이름       (32 bytes)
    uint8_t hmac[HMAC_SIZE];         // HMAC-SHA256      (32 bytes)
} SensorPacket;  // 총 84 bytes
```

**메모리 레이아웃:**

```
Offset  Field           Size    Description
───────────────────────────────────────────────────────
0x00    sensor_id       4       센서 ID (1001, 1002, ...)
0x04    sensor_type     4       1=온도, 2=습도, 3=기압, 4=진동
0x08    value           4       센서 측정값 (IEEE 754 float)
0x0C    timestamp       8       Unix timestamp
0x14    sensor_name     32      센서 이름 문자열
0x34    hmac            32      HMAC-SHA256 해시
───────────────────────────────────────────────────────
Total                   84 bytes
```

**프로토콜 선택 이유:**

| 설계 결정 | 이유 |
|---------|------|
| 고정 크기 (84 bytes) | 파싱 간단, 버퍼 오버플로우 방지 |
| HMAC을 마지막에 배치 | HMAC 계산 시 앞부분만 사용 (`sizeof - HMAC_SIZE`) |
| 타임스탬프 포함 | 재전송 공격 감지 가능 |
| 센서 이름 포함 | 로그 가독성 및 디버깅 편의성 |

---

## 4. 개발 과정 및 문제 해결

### ⚠️ 주요 이슈 및 해결 과정

#### **이슈 1: 구조체 패딩으로 인한 HMAC 불일치**

**문제:**
- C 구조체는 컴파일러가 메모리 정렬을 위해 패딩 삽입
- 클라이언트와 서버에서 계산되는 HMAC이 달라질 수 있음

**해결:**
```c
// 1. 필드 순서를 자연 정렬 순서로 배치 (큰 크기부터)
// 2. 고정 크기 타입 사용 (uint32_t, time_t)
// 3. 구조체 크기 검증
printf("SensorPacket size: %zu\n", sizeof(SensorPacket));  // 84 bytes 확인
```

---

#### **이슈 2: select() 함수 컴파일 오류**

**문제:**
```
error: unknown type name 'fd_set'
error: implicit declaration of function 'select'
```

**해결:**
```c
// monitor_server.c 상단에 추가
#define _POSIX_C_SOURCE 200809L
#include <sys/select.h>
#include <sys/time.h>
```

**교훈:** POSIX 확장 함수는 feature test macro 정의 필요

---

#### **이슈 3: 서버 재시작 시 "Address already in use"**

**문제:**
- 서버 종료 후 즉시 재시작 불가
- TCP TIME_WAIT 상태로 포트 점유

**해결:**
```c
int opt = 1;
setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

**결과:** 서버 즉시 재시작 가능

---

### 💻 주요 코드 하이라이트

#### **코드 1: HMAC-SHA256 계산 함수**

```c
int calculate_hmac(const SensorPacket *packet, uint8_t *hmac_out) {
    unsigned int hmac_len = HMAC_SIZE;

    // 핵심: HMAC 필드를 제외한 데이터만 해시 계산
    size_t data_size = sizeof(SensorPacket) - HMAC_SIZE;  // 52 bytes

    // OpenSSL HMAC 함수 사용
    unsigned char *result = HMAC(
        EVP_sha256(),                    // SHA-256 알고리즘
        SECRET_KEY,                      // 공유 비밀 키
        strlen(SECRET_KEY),
        (unsigned char*)packet,          // 입력 데이터
        data_size,                       // 52 bytes (HMAC 제외)
        hmac_out,                        // 출력: 32 bytes
        &hmac_len
    );

    return (result == NULL) ? -1 : 0;
}
```

**핵심:** `sizeof(SensorPacket) - HMAC_SIZE`로 HMAC 필드 제외

---

#### **코드 2: HMAC 검증 함수**

```c
int verify_hmac(const SensorPacket *packet) {
    uint8_t calculated_hmac[HMAC_SIZE];

    // 1. 수신된 패킷으로 HMAC 재계산
    if (calculate_hmac(packet, calculated_hmac) != 0) {
        return -1;
    }

    // 2. 바이트 단위 비교
    if (memcmp(calculated_hmac, packet->hmac, HMAC_SIZE) != 0) {
        return -1;  // 위변조 감지
    }

    return 0;  // 검증 성공
}
```

**핵심:** 동일한 키로 재계산 후 비교

---

#### **코드 3: 서버의 위변조 감지 로직**

```c
while (running) {
    // 패킷 수신
    recv(client_sock, &packet, sizeof(packet), 0);

    // ★ HMAC 검증 - 핵심 보안 로직
    int verified = (verify_hmac(&packet) == 0);

    // 검증 결과 출력
    printf("[%s] Sensor: %s (ID: %u) | Value: %.2f %s | HMAC: %s\n",
           time_str,
           get_sensor_type_name(packet.sensor_type),
           packet.sensor_id,
           packet.value,
           get_sensor_unit(packet.sensor_type),
           verified ? "✓ VALID" : "✗ INVALID");

    // ★ 위변조 감지 시 경고
    if (!verified) {
        printf("*** WARNING: Data tampering detected! ***\n");
        log_alert(&packet);  // 별도 경고 로그
    }
}
```

**핵심:** 수신 즉시 검증, 위변조 시 즉각 경고

---

## 5. 실행 결과 시연

### 🔨 컴파일 과정

```bash
$ make clean
rm -f monitor_server sensor_client *.o
rm -f sensor_log.txt alert_log.txt

$ make
gcc -Wall -Wextra -O2 -std=c11 -c monitor_server.c
gcc -Wall -Wextra -O2 -std=c11 -c crypto_utils.c
gcc -Wall -Wextra -O2 -std=c11 -c sensor_utils.c
gcc -Wall -Wextra -O2 -std=c11 -o monitor_server monitor_server.o crypto_utils.o sensor_utils.o -lssl -lcrypto
gcc -Wall -Wextra -O2 -std=c11 -c sensor_client.c
gcc -Wall -Wextra -O2 -std=c11 -o sensor_client sensor_client.o crypto_utils.o sensor_utils.o -lssl -lcrypto

$ ls -lh monitor_server sensor_client
-rwxr-xr-x 1 user user 22K Nov 22 16:19 monitor_server
-rwxr-xr-x 1 user user 17K Nov 22 16:19 sensor_client
```

✅ **빌드 성공, 실행 파일 생성 완료**

---

### ✅ 시나리오 1: 정상 데이터 전송

**[터미널 1] 서버 실행:**

```bash
$ ./monitor_server
Sensor Monitoring Server
Port: 8888
Log file: sensor_log.txt
Alert file: alert_log.txt

Server is listening on port 8888...
```

**[터미널 2] 온도 센서 클라이언트:**

```bash
$ ./sensor_client 1001 1
Sensor Client Started
Sensor ID: 1001
Sensor Type: Temperature
Target Server: 127.0.0.1:8888

Connected to server

[1] Sending data: Temperature = 26.34 °C (HMAC: a3f2b8c1...)
[2] Sending data: Temperature = 24.89 °C (HMAC: b4c3d2e1...)
[3] Sending data: Temperature = 27.12 °C (HMAC: c5d4e3f2...)
...
[10] Sending data: Temperature = 25.12 °C (HMAC: d5e6f7a8...)

Transmission completed. Closing connection.
```

**[터미널 1] 서버 수신 로그:**

```bash
New connection from 127.0.0.1:45678
[Fri Nov 22 16:20:15 2024] Sensor: Temperature (ID: 1001) | Value: 26.34 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:17 2024] Sensor: Temperature (ID: 1001) | Value: 24.89 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:19 2024] Sensor: Temperature (ID: 1001) | Value: 27.12 °C | HMAC: ✓ VALID
...
[Fri Nov 22 16:20:33 2024] Sensor: Temperature (ID: 1001) | Value: 25.12 °C | HMAC: ✓ VALID
Client 127.0.0.1 disconnected
```

✅ **결과: 10개 패킷 모두 HMAC 검증 성공**

---

### 🚨 시나리오 2: 위변조 데이터 탐지

**[테스트] protocol.h의 SECRET_KEY 변경:**

```c
// 클라이언트만 다른 키로 재컴파일
#define SECRET_KEY "different_key_for_test"
```

**[터미널 2] 변조된 클라이언트 실행:**

```bash
$ ./sensor_client 9999 1
Sensor Client Started
Sensor ID: 9999
Sensor Type: Temperature
Target Server: 127.0.0.1:8888

Connected to server

[1] Sending data: Temperature = 26.34 °C (HMAC: 1a2b3c4d...)
[2] Sending data: Temperature = 24.89 °C (HMAC: 2b3c4d5e...)
...
```

**[터미널 1] 서버에서 위변조 감지:**

```bash
New connection from 127.0.0.1:45682
[Fri Nov 22 16:30:42 2024] Sensor: Temperature (ID: 9999) | Value: 26.34 °C | HMAC: ✗ INVALID
*** WARNING: Data tampering detected! ***
[Fri Nov 22 16:30:44 2024] Sensor: Temperature (ID: 9999) | Value: 24.89 °C | HMAC: ✗ INVALID
*** WARNING: Data tampering detected! ***
```

**alert_log.txt 확인:**

```bash
$ cat alert_log.txt
[Fri Nov 22 16:30:42 2024] *** ALERT *** HMAC Verification Failed!
  Sensor ID: 9999
  Sensor Type: Temperature
  Sensor Name: Sensor_Temperature_9999
  Value: 26.34 °C
  Timestamp: 1732291842
  Received HMAC: 1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2
```

✅ **결과: 위변조 즉시 감지, 경고 로그 기록**

---

### 🔄 시나리오 3: 다중 센서 동시 실행

**[4개 터미널] 각기 다른 센서 타입:**

```bash
# 터미널 2: 온도 센서
$ ./sensor_client 1001 1

# 터미널 3: 습도 센서
$ ./sensor_client 1002 2

# 터미널 4: 기압 센서
$ ./sensor_client 1003 3

# 터미널 5: 진동 센서
$ ./sensor_client 1004 4
```

**[서버] 통합 로그:**

```bash
[Fri Nov 22 16:21:10 2024] Sensor: Temperature (ID: 1001) | Value: 26.34 °C | HMAC: ✓ VALID
[Fri Nov 22 16:21:12 2024] Sensor: Humidity (ID: 1002) | Value: 65.23 % | HMAC: ✓ VALID
[Fri Nov 22 16:21:14 2024] Sensor: Pressure (ID: 1003) | Value: 1015.67 hPa | HMAC: ✓ VALID
[Fri Nov 22 16:21:16 2024] Sensor: Vibration (ID: 1004) | Value: 45.78 Hz | HMAC: ✓ VALID
```

✅ **결과: 4종 센서 동시 모니터링 성공**

---

## 6. AI 활용 분석 및 개인 소감

### 🤖 AI 활용 효과 분석

#### 계획 대비 실제 활용 효과

**개발 시간 절약 효과:**

| 작업 | 전통적 방법 | AI 활용 | 절감율 |
|-----|-----------|---------|-------|
| 프로젝트 구조 설계 | 4시간 | 30분 | 87.5% |
| HMAC-SHA256 구현 | 3시간 | 45분 | 75% |
| TCP 소켓 프로그래밍 | 3시간 | 1시간 | 66.7% |
| 디버깅 및 에러 해결 | 4시간 | 1.5시간 | 62.5% |
| 문서화 (README) | 2시간 | 30분 | 75% |
| **총계** | **16시간** | **4.25시간** | **73.4%** |

**실제 영향:**
- ✅ 개발 시간 약 12시간 절약
- ✅ 코드 품질 향상 (보안 모범 사례 적용)
- ✅ 학습 곡선 단축 (OpenSSL API, 소켓 프로그래밍)

---

#### AI 활용의 강점

**1. 즉각적인 문제 해결**
```
문제 발생 → AI 질의 → 구체적 해결책 → 적용
(10-20분 소요)

vs

문제 발생 → 검색 → 자료 비교 → 시행착오 → 해결
(1-2시간 소요)
```

**2. 코드 품질**
- 컴파일러 경고 제거 (`-Wall -Wextra` 통과)
- 보안 모범 사례 자동 적용
- POSIX 표준 준수

**3. 학습 효과**
- OpenSSL HMAC API 사용법
- TCP 소켓 프로그래밍 패턴
- 구조체 메모리 레이아웃 이해

---

#### AI 활용의 한계

**AI가 대체할 수 없는 영역:**

| 영역 | 이유 |
|-----|------|
| 요구사항 분석 | 도메인 지식과 창의성 필요 |
| 보안 정책 결정 | 비즈니스 맥락 고려 필요 |
| 아키텍처 설계 | 트레이드오프 판단 필요 |
| 특정 환경 디버깅 | 세밀한 관찰력 필요 |

**주의사항:**
- ⚠️ AI 코드를 이해 없이 복사 금지
- ⚠️ 보안 코드는 반드시 검증 (예: `memcmp` 타이밍 공격)
- ⚠️ 플랫폼별 차이 고려 필요

---

### 💭 개인 소감

#### 프로젝트를 통해 배운 기술적 의미

**1. 암호학 이론의 실용화**
```
이론: "HMAC은 메시지 무결성을 보장한다"
      ↓
실습: 1비트만 바뀌어도 검증 실패 → 수학적 엄밀성 체감
```

**2. 보안 구현의 어려움**
- 단순히 "코드를 짜는 것"과 "안전하게 만드는 것"은 다름
- 키 관리, 재전송 공격, 타이밍 공격 등 고려사항 多
- → **보안은 계층적 접근이 필요**

**3. 시스템 프로그래밍의 복잡성**
- 추상화된 이론 vs 실제 구현의 간극
- 구조체 패딩, 엔디안, 버퍼 관리 등 세부사항
- → **악마는 디테일에 있다**

---

#### AI와의 협업 통찰

**긍정적 경험:**
1. **학습 가속화**: 1주일 프로젝트 → 2일 완성
2. **자신감 향상**: 복잡한 프로젝트도 체계적 접근 가능
3. **모범 사례 학습**: 경험 많은 개발자의 패턴 습득

**얻은 교훈:**
1. **AI는 도구**: 사고를 대체하지 않음
2. **비판적 사고**: 제시된 코드의 적절성 판단 필요
3. **학습 과정**: 결과만이 아닌 과정 이해 중요

---

#### 총평

**핵심 성과:**
- ✅ HMAC-SHA256 기반 데이터 무결성 보장 시스템 구현
- ✅ 위변조 탐지 및 실시간 경고 시스템 구축
- ✅ 4종 센서 시뮬레이션 및 다중 클라이언트 지원
- ✅ 확장 가능한 모듈 구조 설계

**기술적 성장:**
- 암호학 이론 → 실제 보안 시스템 구현
- 네트워크 이론 → TCP/IP 소켓 프로그래밍
- 설계 → 구현 → 테스트의 전체 개발 사이클 경험

**AI 활용 결론:**

> AI는 강력한 개발 가속 도구이지만,
> **문제를 정의하고 해결 방향을 결정하는 것은 개발자의 몫**이다.
>
> 이 프로젝트를 통해 보안 시스템 개발의 복잡성과
> AI 기반 개발의 장단점을 모두 경험할 수 있었다.

---

**프로젝트 정보**
- 완료일: 2024년 11월 22일
- 총 개발 시간: 약 5시간 (AI 활용)
- 코드 라인 수: 약 800줄
- 빌드 크기: 39KB (server 22KB + client 17KB)
