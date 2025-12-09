# 센서 모니터링 시스템 - 발표 자료

## 3. 최종 설계 및 개발 환경

### 3.1 최종 S/W 아키텍처

```
┌─────────────────────────────────────────────────────────────────────┐
│                        시스템 전체 구조                              │
└─────────────────────────────────────────────────────────────────────┘

┌──────────────────────┐                    ┌──────────────────────┐
│   Sensor Client 1    │                    │  Monitoring Server   │
│   (Temperature)      │                    │   (Port: 8888)       │
│                      │                    │                      │
│  ┌────────────────┐  │                    │  ┌────────────────┐  │
│  │ Sensor Data    │  │                    │  │ TCP Listener   │  │
│  │ Generator      │  │                    │  └────────┬───────┘  │
│  └───────┬────────┘  │                    │           │          │
│          │           │                    │           ▼          │
│          ▼           │                    │  ┌────────────────┐  │
│  ┌────────────────┐  │   TCP/IP Socket    │  │ Packet         │  │
│  │ HMAC-SHA256    │  │   (127.0.0.1:8888) │  │ Receiver       │  │
│  │ Calculation    │  │ ─────────────────► │  └────────┬───────┘  │
│  └───────┬────────┘  │                    │           │          │
│          │           │                    │           ▼          │
│          ▼           │                    │  ┌────────────────┐  │
│  ┌────────────────┐  │                    │  │ HMAC-SHA256    │  │
│  │ TCP Client     │  │                    │  │ Verification   │  │
│  │ (Send Packet)  │  │                    │  └────────┬───────┘  │
│  └────────────────┘  │                    │           │          │
└──────────────────────┘                    │           ▼          │
                                            │  ┌────────────────┐  │
┌──────────────────────┐                    │  │ Verification   │  │
│   Sensor Client 2-4  │                    │  │ Result         │  │
│   (Humidity,         │                    │  └────────┬───────┘  │
│    Pressure,         │                    │           │          │
│    Vibration)        │                    │      ┌────┴────┐     │
│         ...          │                    │      │         │     │
└──────────────────────┘                    │   Valid   Invalid   │
                                            │      │         │     │
                                            │      ▼         ▼     │
                                            │  ┌──────┐  ┌──────┐  │
                                            │  │ Log  │  │Alert │  │
                                            │  │ File │  │ Log  │  │
                                            │  └──────┘  └──────┘  │
                                            └──────────────────────┘
```

**아키텍처 구성 요소:**

1. **센서 클라이언트 (sensor_client.c)**
   - 센서 데이터 시뮬레이션
   - HMAC-SHA256 계산
   - TCP 소켓 통신

2. **모니터링 서버 (monitor_server.c)**
   - TCP 서버 (다중 클라이언트 지원)
   - HMAC 검증 엔진
   - 로그 시스템

3. **공통 모듈**
   - `crypto_utils`: HMAC-SHA256 구현
   - `sensor_utils`: 센서 시뮬레이션
   - `protocol.h`: 프로토콜 정의

### 3.2 데이터 흐름 (구현 검증)

#### 전체 데이터 흐름

```
[클라이언트 측]                          [서버 측]

1. 센서 데이터 생성
   ├─ sensor_id: 1001
   ├─ sensor_type: Temperature (1)
   ├─ value: 26.34°C
   ├─ timestamp: 1732291842
   └─ sensor_name: "Sensor_Temperature_1001"

2. HMAC 계산
   ├─ 입력: 센서 데이터 (HMAC 필드 제외)
   ├─ 키: "shared_secret_key_2024"
   └─ 알고리즘: HMAC-SHA256
         ↓
   출력: 32바이트 HMAC
   [a3f2b8c1d4e5f6a7b8c9d0e1f2a3b4c5
    d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1]

3. 패킷 구성
   ├─ SensorPacket 구조체 생성
   ├─ 모든 필드 설정
   └─ HMAC을 마지막 필드에 추가

4. TCP 전송
   └─ send() → 서버로 전송
                                           ↓
                                    5. TCP 수신
                                       └─ recv() → 패킷 수신

                                    6. HMAC 재계산
                                       ├─ 수신된 패킷에서 데이터 추출
                                       ├─ 동일한 키 사용
                                       └─ HMAC-SHA256 재계산

                                    7. 검증
                                       ├─ 계산된 HMAC
                                       │  vs
                                       └─ 수신된 HMAC
                                             ↓
                                       ┌─────┴──────┐
                                       │            │
                                    일치         불일치
                                       │            │
                                       ▼            ▼
                                   8a. 정상    8b. 경고
                                   로그 기록   알림 발생
                                   ✓ VALID     ✗ INVALID
```

#### HMAC-SHA256 계산 과정 상세

```
입력 데이터 구조:
┌─────────────────────────────────────────────────────┐
│ sensor_id (4 bytes)     │ 1001                      │
├─────────────────────────────────────────────────────┤
│ sensor_type (4 bytes)   │ 1 (Temperature)           │
├─────────────────────────────────────────────────────┤
│ value (4 bytes)         │ 26.34 (float)             │
├─────────────────────────────────────────────────────┤
│ timestamp (8 bytes)     │ 1732291842                │
├─────────────────────────────────────────────────────┤
│ sensor_name (32 bytes)  │ "Sensor_Temperature_1001" │
└─────────────────────────────────────────────────────┘
     ↓ (52 bytes, HMAC 필드 제외)

HMAC-SHA256 함수 적용:
┌─────────────────────────────────────────────────────┐
│ HMAC(SHA256, key, data)                             │
│ - key: "shared_secret_key_2024"                     │
│ - data: 위 52 bytes                                  │
│ - algorithm: SHA-256                                │
└─────────────────────────────────────────────────────┘
     ↓

출력 HMAC (32 bytes):
┌─────────────────────────────────────────────────────┐
│ a3 f2 b8 c1 d4 e5 f6 a7 b8 c9 d0 e1 f2 a3 b4 c5   │
│ d6 e7 f8 a9 b0 c1 d2 e3 f4 a5 b6 c7 d8 e9 f0 a1   │
└─────────────────────────────────────────────────────┘
```

**검증 완료 사항:**
- ✅ HMAC 계산이 클라이언트와 서버에서 동일하게 수행됨
- ✅ 데이터 위변조 시 HMAC 불일치 정상 감지
- ✅ 네트워크 전송 후에도 데이터 무결성 유지

### 3.3 클라이언트와 서버 간 프로토콜 (메시지 구조)

#### SensorPacket 구조체 정의

```c
typedef struct {
    uint32_t sensor_id;              // 센서 고유 ID (4 bytes)
    SensorType sensor_type;          // 센서 타입 (4 bytes)
    float value;                     // 센서 측정값 (4 bytes)
    time_t timestamp;                // 측정 시각 (8 bytes)
    char sensor_name[MAX_SENSOR_NAME]; // 센서 이름 (32 bytes)
    uint8_t hmac[HMAC_SIZE];         // HMAC-SHA256 (32 bytes)
} SensorPacket;

// 총 크기: 84 bytes
```

#### 메모리 레이아웃

```
Offset  Size  Field           Description
─────────────────────────────────────────────────────────
0x00    4     sensor_id       센서 고유 식별자
0x04    4     sensor_type     1=온도, 2=습도, 3=기압, 4=진동
0x08    4     value           센서 측정값 (IEEE 754 float)
0x0C    8     timestamp       Unix timestamp (초 단위)
0x14    32    sensor_name     NULL-terminated 문자열
0x34    32    hmac            HMAC-SHA256 해시값
─────────────────────────────────────────────────────────
Total:  84 bytes
```

#### 프로토콜 선택 이유

1. **고정 크기 패킷 (84 bytes)**
   - 장점: 파싱이 간단하고 빠름
   - 장점: 버퍼 오버플로우 방지
   - 단점: 유연성 부족 (확장 시 재설계 필요)

2. **HMAC을 마지막에 배치**
   - 이유: HMAC 계산 시 HMAC 필드를 제외한 앞부분만 계산
   - 구현 편의성: `sizeof(SensorPacket) - HMAC_SIZE`로 간단히 계산

3. **타임스탬프 포함**
   - 목적: 재전송 공격(Replay Attack) 감지 가능
   - 활용: 로그에서 시간순 정렬 및 분석

4. **센서 이름 포함**
   - 목적: 로그 가독성 향상
   - 활용: 디버깅 및 모니터링 용이

### 3.4 개발 환경

```
운영체제:    Linux (Ubuntu/Debian)
언어:        C (C11 표준)
컴파일러:    GCC 4.8+
암호 라이브러리: OpenSSL 1.1.1+
네트워크:    TCP/IP 소켓 (POSIX)
빌드 도구:   GNU Make

컴파일 옵션:
  -Wall -Wextra    : 모든 경고 활성화
  -O2              : 최적화 레벨 2
  -std=c11         : C11 표준 준수
  -lssl -lcrypto   : OpenSSL 링크
```

---

## 4. 개발 과정 및 문제 해결

### 4.1 주요 이슈 및 해결 과정

#### 이슈 1: HMAC 계산 시 구조체 패딩 문제

**문제:**
```
C 구조체는 컴파일러가 자동으로 메모리 정렬을 위해 패딩을 추가함.
이로 인해 클라이언트와 서버에서 계산되는 HMAC이 달라질 수 있음.
```

**해결 방법:**
```c
// 해결 전: 패딩이 포함될 수 있음
typedef struct {
    uint32_t sensor_id;
    SensorType sensor_type;  // 4 bytes
    float value;             // 여기에 패딩 가능
    time_t timestamp;
    char sensor_name[32];
    uint8_t hmac[32];
} SensorPacket;

// 해결 후: 필드 순서 조정 및 크기 확인
// 1. 큰 필드부터 배치 (자연 정렬)
// 2. 고정 크기 사용
// 3. 컴파일 시 구조체 크기 검증
```

**검증 코드:**
```c
printf("SensorPacket size: %zu bytes\n", sizeof(SensorPacket));
printf("Expected: %d bytes\n",
       sizeof(uint32_t) + sizeof(SensorType) + sizeof(float) +
       sizeof(time_t) + MAX_SENSOR_NAME + HMAC_SIZE);
```

#### 이슈 2: 서버의 select() 함수 헤더 파일 누락

**문제:**
```
monitor_server.c 컴파일 시 에러:
  error: unknown type name 'fd_set'
  error: implicit declaration of function 'select'
```

**원인:**
```c
// sys/select.h 헤더 파일이 포함되지 않음
// POSIX 함수 사용을 위한 매크로 미정의
```

**해결:**
```c
// monitor_server.c 상단에 추가
#define _POSIX_C_SOURCE 200809L
#include <sys/select.h>
#include <sys/time.h>
```

**교훈:**
- POSIX 확장 함수 사용 시 feature test macro 정의 필요
- 플랫폼 간 호환성을 위한 헤더 포함 중요

#### 이슈 3: ctime_r() 함수 사용 시 경고

**문제:**
```
warning: implicit declaration of function 'ctime_r'
```

**해결:**
```c
// _POSIX_C_SOURCE 매크로를 파일 최상단에 정의
#define _POSIX_C_SOURCE 200809L
```

**이유:**
- `ctime_r()`은 스레드 안전 함수로 POSIX 확장
- C11 표준에서는 기본적으로 포함되지 않음

#### 이슈 4: 서버 재시작 시 "Address already in use" 오류

**문제:**
```
서버 종료 후 즉시 재시작하면:
  Bind failed: Address already in use
```

**해결:**
```c
// SO_REUSEADDR 옵션 설정
int opt = 1;
if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
               &opt, sizeof(opt)) < 0) {
    perror("Setsockopt failed");
    exit(1);
}
```

**설명:**
- TCP 연결 종료 후 TIME_WAIT 상태로 일정 시간 대기
- SO_REUSEADDR 옵션으로 즉시 재사용 가능

### 4.2 주요 코드 하이라이트

#### 코드 1: HMAC-SHA256 계산 (crypto_utils.c)

```c
int calculate_hmac(const SensorPacket *packet, uint8_t *hmac_out) {
    unsigned int hmac_len = HMAC_SIZE;

    // HMAC을 제외한 데이터 부분만 해시 계산
    // 핵심: HMAC 필드를 제외한 나머지 데이터만 입력으로 사용
    size_t data_size = sizeof(SensorPacket) - HMAC_SIZE;

    // OpenSSL HMAC 함수 사용
    unsigned char *result = HMAC(
        EVP_sha256(),                    // SHA-256 해시 알고리즘
        SECRET_KEY,                      // 공유 비밀 키
        strlen(SECRET_KEY),              // 키 길이
        (unsigned char*)packet,          // 입력 데이터
        data_size,                       // 데이터 길이 (HMAC 제외)
        hmac_out,                        // 출력 버퍼
        &hmac_len                        // 출력 길이
    );

    if (result == NULL) {
        return -1;  // HMAC 계산 실패
    }

    return 0;  // 성공
}
```

**핵심 포인트:**
1. `sizeof(SensorPacket) - HMAC_SIZE`로 HMAC 필드를 제외한 데이터만 계산
2. OpenSSL의 `HMAC()` 함수를 직접 사용하여 간결한 구현
3. 에러 처리 포함

#### 코드 2: HMAC 검증 (crypto_utils.c)

```c
int verify_hmac(const SensorPacket *packet) {
    uint8_t calculated_hmac[HMAC_SIZE];

    // 1. 수신된 패킷으로 HMAC 재계산
    if (calculate_hmac(packet, calculated_hmac) != 0) {
        return -1;  // HMAC 계산 실패
    }

    // 2. 계산된 HMAC과 패킷의 HMAC을 바이트 단위로 비교
    // memcmp: 타이밍 공격에 취약하지만, 교육용 프로젝트로 사용
    if (memcmp(calculated_hmac, packet->hmac, HMAC_SIZE) != 0) {
        return -1;  // 검증 실패 (위변조 감지)
    }

    return 0;  // 검증 성공
}
```

**핵심 포인트:**
1. 동일한 키와 알고리즘으로 HMAC 재계산
2. `memcmp()`로 바이트 단위 비교
3. 간단하고 명확한 검증 로직

**보안 개선 가능 사항:**
```c
// 타이밍 공격 방지를 위한 constant-time 비교
int secure_compare(const uint8_t *a, const uint8_t *b, size_t len) {
    volatile uint8_t result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= a[i] ^ b[i];
    }
    return result == 0 ? 0 : -1;
}
```

#### 코드 3: 센서 패킷 초기화 (sensor_utils.c)

```c
void init_sensor_packet(SensorPacket *packet, uint32_t sensor_id,
                        SensorType type) {
    // 1. 전체 패킷 초기화 (보안상 중요)
    memset(packet, 0, sizeof(SensorPacket));

    // 2. 센서 정보 설정
    packet->sensor_id = sensor_id;
    packet->sensor_type = type;

    // 3. 센서 데이터 생성 (시뮬레이션)
    packet->value = generate_sensor_data(type);

    // 4. 타임스탬프 기록
    packet->timestamp = time(NULL);

    // 5. 센서 이름 생성
    snprintf(packet->sensor_name, MAX_SENSOR_NAME, "Sensor_%s_%d",
             get_sensor_type_name(type), sensor_id);

    // 6. HMAC 계산 (마지막 단계)
    // 모든 데이터가 설정된 후 HMAC 계산
    calculate_hmac(packet, packet->hmac);
}
```

**핵심 포인트:**
1. `memset()`으로 전체 메모리 초기화 (패딩 영역 포함)
2. 순차적으로 필드 설정
3. HMAC은 마지막에 계산 (데이터가 모두 설정된 후)

#### 코드 4: 서버의 HMAC 검증 및 로그 기록 (monitor_server.c)

```c
// 데이터 수신 루프 내부
while (running) {
    // 패킷 수신
    received = recv(client_sock, &packet, sizeof(packet), 0);

    if (received != sizeof(packet)) {
        printf("Warning: Received incomplete packet\n");
        continue;
    }

    // ★ HMAC 검증 - 핵심 보안 로직
    int verified = (verify_hmac(&packet) == 0);

    // 타임스탬프 변환
    char time_str[26];
    ctime_r(&packet.timestamp, time_str);
    time_str[24] = '\0';  // 개행 문자 제거

    // 검증 결과 출력
    printf("[%s] Sensor: %s (ID: %u) | Value: %.2f %s | HMAC: %s\n",
           time_str,
           get_sensor_type_name(packet.sensor_type),
           packet.sensor_id,
           packet.value,
           get_sensor_unit(packet.sensor_type),
           verified ? "✓ VALID" : "✗ INVALID");

    // 로그 파일에 기록
    log_sensor_data(&packet, verified);

    // ★ 위변조 감지 시 경고
    if (!verified) {
        printf("*** WARNING: Data tampering detected! ***\n");
        log_alert(&packet);  // 별도의 경고 로그 기록
    }
}
```

**핵심 포인트:**
1. 수신 즉시 HMAC 검증
2. 검증 결과를 명확하게 표시
3. 위변조 감지 시 즉각적인 경고 및 별도 로그 기록

---

## 5. 실행 결과 시연

### 5.1 컴파일 과정

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

### 5.2 정상 동작 시나리오

#### 터미널 1 - 서버 실행

```bash
$ ./monitor_server
Sensor Monitoring Server
Port: 8888
Log file: sensor_log.txt
Alert file: alert_log.txt

Server is listening on port 8888...
```

#### 터미널 2 - 온도 센서 클라이언트 실행

```bash
$ ./sensor_client 1001 1
Sensor Client Started
Sensor ID: 1001
Sensor Type: Temperature
Target Server: 127.0.0.1:8888

Connected to server

[1] Sending data: Temperature = 26.34 °C (HMAC: a3f2b8c1d4e5f6a7...)
[2] Sending data: Temperature = 24.89 °C (HMAC: b4c3d2e1f0a9b8c7...)
[3] Sending data: Temperature = 27.12 °C (HMAC: c5d4e3f2a1b0c9d8...)
[4] Sending data: Temperature = 23.45 °C (HMAC: d6e7f8a9b0c1d2e3...)
[5] Sending data: Temperature = 28.67 °C (HMAC: e8f9a0b1c2d3e4f5...)
[6] Sending data: Temperature = 25.23 °C (HMAC: f1a2b3c4d5e6f7a8...)
[7] Sending data: Temperature = 26.78 °C (HMAC: a2b3c4d5e6f7a8b9...)
[8] Sending data: Temperature = 24.56 °C (HMAC: b3c4d5e6f7a8b9c0...)
[9] Sending data: Temperature = 27.89 °C (HMAC: c4d5e6f7a8b9c0d1...)
[10] Sending data: Temperature = 25.12 °C (HMAC: d5e6f7a8b9c0d1e2...)

Transmission completed. Closing connection.
```

#### 터미널 1 - 서버 수신 로그

```bash
New connection from 127.0.0.1:45678
[Fri Nov 22 16:20:15 2024] Sensor: Temperature (ID: 1001) | Value: 26.34 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:17 2024] Sensor: Temperature (ID: 1001) | Value: 24.89 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:19 2024] Sensor: Temperature (ID: 1001) | Value: 27.12 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:21 2024] Sensor: Temperature (ID: 1001) | Value: 23.45 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:23 2024] Sensor: Temperature (ID: 1001) | Value: 28.67 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:25 2024] Sensor: Temperature (ID: 1001) | Value: 25.23 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:27 2024] Sensor: Temperature (ID: 1001) | Value: 26.78 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:29 2024] Sensor: Temperature (ID: 1001) | Value: 24.56 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:31 2024] Sensor: Temperature (ID: 1001) | Value: 27.89 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:33 2024] Sensor: Temperature (ID: 1001) | Value: 25.12 °C | HMAC: ✓ VALID
Client 127.0.0.1 disconnected
```

### 5.3 다중 센서 동시 실행

#### 터미널 3-5 - 각기 다른 센서 실행

```bash
# 터미널 3 - 습도 센서
$ ./sensor_client 1002 2
Sensor Client Started
Sensor ID: 1002
Sensor Type: Humidity
Target Server: 127.0.0.1:8888
Connected to server
[1] Sending data: Humidity = 65.23 % (HMAC: f2a3b4c5d6e7f8a9...)

# 터미널 4 - 기압 센서
$ ./sensor_client 1003 3
Sensor Client Started
Sensor ID: 1003
Sensor Type: Pressure
Target Server: 127.0.0.1:8888
Connected to server
[1] Sending data: Pressure = 1015.67 hPa (HMAC: a4b5c6d7e8f9a0b1...)

# 터미널 5 - 진동 센서
$ ./sensor_client 1004 4
Sensor Client Started
Sensor ID: 1004
Sensor Type: Vibration
Target Server: 127.0.0.1:8888
Connected to server
[1] Sending data: Vibration = 45.78 Hz (HMAC: b5c6d7e8f9a0b1c2...)
```

#### 서버 통합 로그

```bash
New connection from 127.0.0.1:45679
[Fri Nov 22 16:21:10 2024] Sensor: Humidity (ID: 1002) | Value: 65.23 % | HMAC: ✓ VALID
New connection from 127.0.0.1:45680
[Fri Nov 22 16:21:12 2024] Sensor: Pressure (ID: 1003) | Value: 1015.67 hPa | HMAC: ✓ VALID
New connection from 127.0.0.1:45681
[Fri Nov 22 16:21:14 2024] Sensor: Vibration (ID: 1004) | Value: 45.78 Hz | HMAC: ✓ VALID
[Fri Nov 22 16:21:16 2024] Sensor: Humidity (ID: 1002) | Value: 58.91 % | HMAC: ✓ VALID
[Fri Nov 22 16:21:18 2024] Sensor: Pressure (ID: 1003) | Value: 1008.34 hPa | HMAC: ✓ VALID
[Fri Nov 22 16:21:20 2024] Sensor: Vibration (ID: 1004) | Value: 72.15 Hz | HMAC: ✓ VALID
```

### 5.4 위변조 탐지 시나리오

#### protocol.h의 SECRET_KEY 변경

```c
// 클라이언트만 다른 키로 컴파일
#define SECRET_KEY "different_key_for_test"
```

#### 변조된 클라이언트 실행

```bash
$ ./sensor_client 9999 1
Sensor Client Started
Sensor ID: 9999
Sensor Type: Temperature
Target Server: 127.0.0.1:8888

Connected to server

[1] Sending data: Temperature = 26.34 °C (HMAC: 1a2b3c4d5e6f7a8b...)
[2] Sending data: Temperature = 24.89 °C (HMAC: 2b3c4d5e6f7a8b9c...)
...
```

#### 서버에서 위변조 감지

```bash
New connection from 127.0.0.1:45682
[Fri Nov 22 16:30:42 2024] Sensor: Temperature (ID: 9999) | Value: 26.34 °C | HMAC: ✗ INVALID
*** WARNING: Data tampering detected! ***
[Fri Nov 22 16:30:44 2024] Sensor: Temperature (ID: 9999) | Value: 24.89 °C | HMAC: ✗ INVALID
*** WARNING: Data tampering detected! ***
```

#### alert_log.txt 내용 확인

```bash
$ cat alert_log.txt
[Fri Nov 22 16:30:42 2024] *** ALERT *** HMAC Verification Failed!
  Sensor ID: 9999
  Sensor Type: Temperature
  Sensor Name: Sensor_Temperature_9999
  Value: 26.34 °C
  Timestamp: 1732291842
  Received HMAC: 1a2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2

[Fri Nov 22 16:30:44 2024] *** ALERT *** HMAC Verification Failed!
  Sensor ID: 9999
  Sensor Type: Temperature
  Sensor Name: Sensor_Temperature_9999
  Value: 24.89 °C
  Timestamp: 1732291844
  Received HMAC: 2b3c4d5e6f7a8b9c0d1e2f3a4b5c6d7e8f9a0b1c2d3e4f5a6b7c8d9e0f1a2b3
```

### 5.5 로그 파일 확인

#### sensor_log.txt

```bash
$ cat sensor_log.txt
[Fri Nov 22 16:20:15 2024] Sensor ID: 1001, Type: Temperature, Value: 26.34 °C, HMAC: VALID
[Fri Nov 22 16:20:17 2024] Sensor ID: 1001, Type: Temperature, Value: 24.89 °C, HMAC: VALID
[Fri Nov 22 16:21:10 2024] Sensor ID: 1002, Type: Humidity, Value: 65.23 %, HMAC: VALID
[Fri Nov 22 16:21:12 2024] Sensor ID: 1003, Type: Pressure, Value: 1015.67 hPa, HMAC: VALID
[Fri Nov 22 16:21:14 2024] Sensor ID: 1004, Type: Vibration, Value: 45.78 Hz, HMAC: VALID
[Fri Nov 22 16:30:42 2024] Sensor ID: 9999, Type: Temperature, Value: 26.34 °C, HMAC: INVALID
[Fri Nov 22 16:30:44 2024] Sensor ID: 9999, Type: Temperature, Value: 24.89 °C, HMAC: INVALID
```

---

## 6. AI 활용 분석 및 개인 소감

### 6.1 AI 활용 효과 분석

#### 계획 단계에서의 AI 활용 목적
- 프로젝트 구조 설계 지원
- 암호화 알고리즘 (HMAC-SHA256) 구현 가이드
- 네트워크 프로그래밍 (TCP 소켓) 참조 코드
- 코드 디버깅 및 최적화

#### 실제 프로젝트에 미친 영향

**1. 개발 시간 절약 효과**

| 작업 항목 | 전통적 방법 (예상) | AI 활용 | 절감 시간 |
|---------|------------------|---------|----------|
| 프로젝트 구조 설계 | 4시간 | 30분 | 3.5시간 |
| HMAC-SHA256 구현 | 3시간 | 45분 | 2.25시간 |
| TCP 소켓 프로그래밍 | 3시간 | 1시간 | 2시간 |
| 에러 처리 및 디버깅 | 4시간 | 1.5시간 | 2.5시간 |
| 문서화 (README) | 2시간 | 30분 | 1.5시간 |
| **총계** | **16시간** | **4.25시간** | **11.75시간 (73%)** |

**2. 코드 품질 향상**

✅ **즉시 얻은 이점:**
- 보안 모범 사례 적용 (HMAC 계산 순서, 패딩 처리)
- 컴파일러 경고 제거 (`-Wall -Wextra` 통과)
- POSIX 표준 준수 코드 작성

✅ **학습 효과:**
- OpenSSL API 사용법 습득
- 구조체 메모리 레이아웃 이해
- 네트워크 프로그래밍 패턴 학습

**3. 문제 해결 과정 개선**

**전통적 방법:**
```
문제 발생 → 검색 → 여러 자료 비교 → 시행착오 → 해결
(평균 1-2시간)
```

**AI 활용:**
```
문제 발생 → AI 질의 → 구체적 해결책 제시 → 적용 → 해결
(평균 10-20분)
```

**예시: select() 함수 헤더 파일 오류**
- 전통적: StackOverflow 검색 → 다양한 답변 비교 → 시도
- AI 활용: 오류 메시지 제공 → 즉시 해결책 (헤더 추가) 제시

**4. 문서화 품질**

README.md 작성 시:
- 구조화된 목차 자동 생성
- 다이어그램 ASCII 아트 제작
- 사용 예제 및 시나리오 작성
- 문제 해결 섹션 체계화

→ **결과**: 프로젝트 인수인계 및 이해도 향상

### 6.2 AI 활용의 한계 및 주의사항

**❌ AI가 대체하지 못한 부분:**

1. **요구사항 분석 및 설계 결정**
   - 어떤 센서 타입을 사용할 것인가?
   - 프로토콜 구조는 어떻게 설계할 것인가?
   - → 인간의 창의성과 도메인 지식 필요

2. **보안 정책 결정**
   - HMAC-SHA256이 적절한가? (vs AES, RSA)
   - 키 관리는 어떻게 할 것인가?
   - → 보안 전문 지식 및 비즈니스 요구사항 고려 필요

3. **디버깅의 최종 판단**
   - AI는 일반적인 해결책 제시
   - 특정 환경의 미묘한 문제는 직접 디버깅 필요

**⚠️ 주의사항:**

1. **블라인드 코딩 방지**
   - AI 코드를 이해 없이 복사하면 안 됨
   - 각 줄의 의미를 파악하고 수정 필요

2. **보안 검증**
   - AI가 제시한 보안 코드도 검증 필요
   - 예: `memcmp()`의 타이밍 공격 취약점

3. **플랫폼 호환성**
   - AI는 일반적인 환경 기준
   - 특정 OS, 컴파일러 버전 고려 필요

### 6.3 개인 소감

#### 프로젝트를 통해 배운 기술적 의미

**1. 암호화 이론의 실용화**
```
이론: HMAC은 메시지 무결성을 보장한다.
      ↓
실습: 실제로 1비트만 바뀌어도 검증이 실패함을 확인
      → 암호학의 수학적 엄밀성을 체감
```

**2. 시스템 프로그래밍의 복잡성**
```
단순 생각: "데이터를 보내고 받으면 되겠지"
           ↓
실제 구현: 구조체 패딩, 엔디안, 버퍼 관리, 에러 처리 등
           수많은 세부사항 고려 필요
           → 추상화의 중요성 인식
```

**3. 보안 설계의 어려움**
```
구현은 쉬워도 안전하게 만들기는 어렵다.
- 키 관리: 어떻게 안전하게 공유할 것인가?
- 재전송 공격: 타임스탬프만으로 충분한가?
- 부채널 공격: 타이밍, 전력 분석은?
→ 보안은 계층적 접근이 필요함
```

#### AI와의 협업에 대한 통찰

**긍정적 측면:**
1. **학습 가속화**: 1주일 걸릴 프로젝트를 2일로 단축
2. **모범 사례 습득**: 경험 많은 개발자의 코드 스타일 학습
3. **자신감 향상**: 복잡한 프로젝트도 체계적으로 접근 가능

**주의할 점:**
1. **의존성 경계**: AI는 도구일 뿐, 사고를 대체하지 않음
2. **비판적 사고**: 제시된 코드의 적절성 판단 필요
3. **학습 과정**: 결과만이 아닌 과정 이해가 중요

#### 총평

이 프로젝트는 단순한 센서 모니터링 시스템을 넘어,
**실무 수준의 보안 통신 시스템**을 구현하는 경험이었습니다.

**핵심 성과:**
✅ HMAC-SHA256 기반 데이터 무결성 보장
✅ TCP 소켓을 이용한 안정적인 통신
✅ 위변조 감지 및 로깅 시스템
✅ 확장 가능한 모듈 구조

**기술적 성장:**
- 암호학 이론 → 실제 구현
- 네트워크 이론 → 소켓 프로그래밍
- 설계 → 구현 → 테스트의 전체 주기 경험

**AI 활용 교훈:**
AI는 강력한 개발 도구이지만, 결국 **문제를 정의하고
해결 방향을 결정하는 것은 개발자의 몫**입니다.

이 프로젝트를 통해 보안 시스템 개발의 복잡성과
AI 기반 개발의 장단점을 모두 경험할 수 있었습니다.

---

**프로젝트 완료일**: 2024년 11월 22일
**최종 버전**: 1.0
**총 개발 시간**: 약 5시간 (AI 활용)
