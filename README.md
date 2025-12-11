# 센서 모니터링 시스템

HMAC-SHA256 기반의 안전한 센서 데이터 전송 및 모니터링 시스템입니다.

## 목차
- [개요](#개요)
- [주요 기능](#주요-기능)
- [시스템 아키텍처](#시스템-아키텍처)
- [파일 구조](#파일-구조)
- [요구 사항](#요구-사항)
- [설치 및 빌드](#설치-및-빌드)
- [실행 방법](#실행-방법)
- [사용 예제](#사용-예제)
- [위변조 테스트](#위변조-테스트)
- [로그 확인](#로그-확인)
- [문제 해결](#문제-해결)
- [코드 구조 설명](#코드-구조-설명)
- [보안 메커니즘](#보안-메커니즘)

## 개요

이 프로젝트는 IoT 센서 환경에서 데이터 무결성을 보장하기 위한 보안 통신 시스템입니다.
센서에서 수집된 데이터가 전송 중 위변조되지 않았음을 HMAC-SHA256 알고리즘을 통해 검증합니다.

### 동작 원리
1. **센서 클라이언트**: 센서 데이터 생성 → HMAC-SHA256 계산 → 서버로 전송
2. **모니터링 서버**: 데이터 수신 → HMAC 재계산 및 검증 → 로그 저장
3. **위변조 감지**: HMAC 불일치 시 경고 발생 및 별도 로그 기록

## 주요 기능

- ✅ **센서 클라이언트**: 센서 데이터 생성, HMAC-SHA256 계산, TCP 전송
- ✅ **모니터링 서버**: TCP 수신, HMAC 검증, 로그 저장
- ✅ **4가지 센서 타입**: 온도, 습도, 기압, 진동
- ✅ **위변조 탐지**: HMAC 검증 실패 시 경고 및 별도 로그 기록
- ✅ **실시간 모니터링**: 센서 데이터 실시간 출력 및 로그 저장
- ✅ **안전한 통신**: OpenSSL 기반 HMAC-SHA256 사용

## 시스템 아키텍처

```
┌─────────────────────┐                    ┌─────────────────────┐
│  Sensor Client 1    │                    │  Monitoring Server  │
│  (Temperature)      │──────┐            ┌│  Port: 8888         │
└─────────────────────┘      │            │└─────────────────────┘
                             │   TCP/IP   │          │
┌─────────────────────┐      │            │          │
│  Sensor Client 2    │──────┼────────────┤          │
│  (Humidity)         │      │            │          ▼
└─────────────────────┘      │            │   ┌─────────────┐
                             │            │   │  HMAC-SHA256│
┌─────────────────────┐      │            │   │  Verification│
│  Sensor Client 3    │──────┤            │   └─────────────┘
│  (Pressure)         │      │            │          │
└─────────────────────┘      │            │          ▼
                             │            │   ┌─────────────┐
┌─────────────────────┐      │            │   │   Logging   │
│  Sensor Client 4    │──────┘            │   │   System    │
│  (Vibration)        │                   │   └─────────────┘
└─────────────────────┘                   │          │
                                          │          ▼
      Each sends:                         │   ┌─────────────┐
      • Sensor Data                       │   │ sensor_log  │
      • HMAC-SHA256                       │   │ alert_log   │
                                          └───┴─────────────┘
```

## 파일 구조

```
Crypto/
├── protocol.h          # 프로토콜 구조체 및 상수 정의
│                       # - SensorPacket 구조체
│                       # - SensorType enum
│                       # - HMAC_SIZE, SECRET_KEY 정의
│
├── crypto_utils.h      # HMAC 관련 함수 헤더
├── crypto_utils.c      # HMAC-SHA256 구현
│                       # - calculate_hmac(): HMAC 계산
│                       # - verify_hmac(): HMAC 검증
│
├── sensor_utils.h      # 센서 시뮬레이션 함수 헤더
├── sensor_utils.c      # 센서 데이터 생성 로직
│                       # - generate_sensor_data(): 센서값 생성
│                       # - init_sensor_packet(): 패킷 초기화
│                       # - get_sensor_type_name(): 타입명 반환
│
├── sensor_client.c     # 센서 클라이언트 메인 프로그램
│                       # - TCP 클라이언트 구현
│                       # - 센서 데이터 전송
│
├── monitor_server.c    # 모니터링 서버 메인 프로그램
│                       # - TCP 서버 구현
│                       # - HMAC 검증
│                       # - 로그 기록
│
├── Makefile           # 빌드 자동화 스크립트
├── .gitignore         # Git 제외 파일 설정
└── README.md          # 프로젝트 문서 (본 파일)
```

## 요구 사항

### 필수 소프트웨어
- **GCC 컴파일러**: 버전 4.8 이상
- **OpenSSL 라이브러리**: libssl-dev (HMAC-SHA256 계산용)
- **Make**: 빌드 자동화

### 운영체제
- Linux (Ubuntu, Debian, CentOS 등)
- macOS
- Windows (WSL 환경)

### Ubuntu/Debian에서 설치

```bash
# 패키지 목록 업데이트
sudo apt-get update

# 필수 패키지 설치
sudo apt-get install -y build-essential libssl-dev

# 설치 확인
gcc --version
openssl version
```

### CentOS/RHEL에서 설치

```bash
sudo yum groupinstall "Development Tools"
sudo yum install openssl-devel
```

### macOS에서 설치

```bash
# Homebrew 사용
brew install openssl

# 컴파일 시 OpenSSL 경로 지정 필요할 수 있음
export CFLAGS="-I/usr/local/opt/openssl/include"
export LDFLAGS="-L/usr/local/opt/openssl/lib"
```

## 설치 및 빌드

### 1. 프로젝트 다운로드

```bash
# Git 저장소 클론
git clone <repository-url>
cd Crypto
```

### 2. 빌드

```bash
# 전체 빌드
make

# 빌드 성공 시 출력 예시:
# gcc -Wall -Wextra -O2 -std=c11 -c monitor_server.c
# gcc -Wall -Wextra -O2 -std=c11 -c crypto_utils.c
# gcc -Wall -Wextra -O2 -std=c11 -c sensor_utils.c
# gcc -Wall -Wextra -O2 -std=c11 -o monitor_server monitor_server.o crypto_utils.o sensor_utils.o -lssl -lcrypto
# gcc -Wall -Wextra -O2 -std=c11 -c sensor_client.c
# gcc -Wall -Wextra -O2 -std=c11 -o sensor_client sensor_client.o crypto_utils.o sensor_utils.o -lssl -lcrypto
```

### 3. 빌드 결과 확인

```bash
ls -lh monitor_server sensor_client

# 예상 출력:
# -rwxr-xr-x 1 user user 22K Nov 22 16:19 monitor_server
# -rwxr-xr-x 1 user user 17K Nov 22 16:19 sensor_client
```

### 4. 클린 빌드

```bash
# 모든 빌드 파일 삭제
make clean

# 다시 빌드
make all
```

## 실행 방법

### 기본 실행 (단계별 가이드)

#### STEP 1: 터미널 준비

최소 2개의 터미널이 필요합니다:
- **터미널 1**: 모니터링 서버 실행
- **터미널 2-5**: 센서 클라이언트 실행 (1개 이상)

#### STEP 2: 서버 실행

**터미널 1에서:**

```bash
./monitor_server
```

**예상 출력:**
```
Sensor Monitoring Server
Port: 8888
Log file: sensor_log.txt
Alert file: alert_log.txt

Server is listening on port 8888...
```

서버가 정상적으로 시작되면 위와 같은 메시지가 출력됩니다.
이제 서버는 포트 8888에서 클라이언트 연결을 대기합니다.

#### STEP 3: 클라이언트 실행

**터미널 2에서 (온도 센서):**

```bash
./sensor_client 1001 1
```

**예상 출력:**
```
Sensor Client Started
Sensor ID: 1001
Sensor Type: Temperature
Target Server: 127.0.0.1:8888

Connected to server

[1] Sending data: Temperature = 26.34 °C (HMAC: a3f2b8c1d4e5f6a7...)
[2] Sending data: Temperature = 24.89 °C (HMAC: b4c3d2e1f0a9b8c7...)
[3] Sending data: Temperature = 27.12 °C (HMAC: c5d4e3f2a1b0c9d8...)
...
[10] Sending data: Temperature = 25.67 °C (HMAC: e7f6a5b4c3d2e1f0...)

Transmission completed. Closing connection.
```

**터미널 1 (서버)에서 확인:**
```
New connection from 127.0.0.1:45678
[Fri Nov 22 16:20:15 2024] Sensor: Temperature (ID: 1001) | Value: 26.34 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:17 2024] Sensor: Temperature (ID: 1001) | Value: 24.89 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:19 2024] Sensor: Temperature (ID: 1001) | Value: 27.12 °C | HMAC: ✓ VALID
...
Client 127.0.0.1 disconnected
```

#### STEP 4: 여러 센서 동시 실행

다른 터미널에서 추가 센서를 실행할 수 있습니다:

**터미널 3 (습도 센서):**
```bash
./sensor_client 1002 2
```

**터미널 4 (기압 센서):**
```bash
./sensor_client 1003 3
```

**터미널 5 (진동 센서):**
```bash
./sensor_client 1004 4
```

### 명령줄 인자 설명

```bash
./sensor_client <sensor_id> <sensor_type>
```

| 파라미터 | 설명 | 예시 |
|---------|------|------|
| sensor_id | 센서 고유 ID (1 이상의 정수) | 1001, 1002, 2001 등 |
| sensor_type | 센서 타입 (1-4) | 1=온도, 2=습도, 3=기압, 4=진동 |

**기본값:**
- sensor_id: 1001
- sensor_type: 1 (Temperature)

```bash
# 기본값으로 실행 (센서 ID: 1001, 타입: Temperature)
./sensor_client

# 센서 ID만 지정 (타입은 Temperature)
./sensor_client 2001

# 센서 ID와 타입 모두 지정
./sensor_client 3001 3
```

## 사용 예제

### 예제 1: 단일 온도 센서 모니터링

```bash
# 터미널 1: 서버 실행
./monitor_server

# 터미널 2: 온도 센서 실행
./sensor_client 1001 1
```

### 예제 2: 4가지 센서 타입 동시 모니터링

```bash
# 터미널 1: 서버 실행
./monitor_server

# 터미널 2-5: 각 센서 실행
./sensor_client 1001 1  # 온도
./sensor_client 1002 2  # 습도
./sensor_client 1003 3  # 기압
./sensor_client 1004 4  # 진동
```

### 예제 3: 같은 타입의 여러 센서

```bash
# 서버 실행
./monitor_server

# 여러 온도 센서 실행 (다른 ID 사용)
./sensor_client 1001 1  # 온도 센서 #1
./sensor_client 1002 1  # 온도 센서 #2
./sensor_client 1003 1  # 온도 센서 #3
```

### 예제 4: Makefile 타겟 사용

```bash
# 서버 빌드 및 실행
make run-server

# 클라이언트 빌드 및 실행 (다른 터미널에서)
make run-client

# 데모 실행 방법 확인
make demo

# 도움말 표시
make help
```

## 센서 타입 상세 정보

| 타입 번호 | 센서 타입 | 데이터 범위 | 단위 | 설명 |
|----------|----------|------------|------|------|
| 1 | Temperature (온도) | 20.0 ~ 30.0 | °C | 실내 온도 시뮬레이션 |
| 2 | Humidity (습도) | 40.0 ~ 80.0 | % | 상대 습도 시뮬레이션 |
| 3 | Pressure (기압) | 983.25 ~ 1043.25 | hPa | 대기압 시뮬레이션 |
| 4 | Vibration (진동) | 0.0 ~ 100.0 | Hz | 기계 진동 주파수 시뮬레이션 |

각 센서는 지정된 범위 내에서 랜덤한 값을 생성합니다.

## 위변조 테스트

시스템의 위변조 탐지 기능을 테스트하려면 다음 방법을 사용할 수 있습니다:

### 방법 1: SECRET_KEY 변경

1. **클라이언트의 SECRET_KEY 변경**

```bash
# protocol.h 편집
nano protocol.h

# SECRET_KEY를 다른 값으로 변경
#define SECRET_KEY "different_key"
```

2. **클라이언트만 재빌드**

```bash
# 클라이언트만 재컴파일
gcc -Wall -Wextra -O2 -std=c11 -c sensor_client.c
gcc -Wall -Wextra -O2 -std=c11 -c crypto_utils.c
gcc -Wall -Wextra -O2 -std=c11 -c sensor_utils.c
gcc -Wall -Wextra -O2 -std=c11 -o sensor_client sensor_client.o crypto_utils.o sensor_utils.o -lssl -lcrypto
```

3. **서버 실행 (원래 키 사용)**

```bash
./monitor_server
```

4. **변조된 클라이언트 실행**

```bash
./sensor_client 9999 1
```

5. **서버 출력 확인**

```
[Fri Nov 22 16:30:42 2024] Sensor: Temperature (ID: 9999) | Value: 26.34 °C | HMAC: ✗ INVALID
*** WARNING: Data tampering detected! ***
```

6. **alert_log.txt 확인**

```bash
cat alert_log.txt
```

**예상 출력:**
```
[Fri Nov 22 16:30:42 2024] *** ALERT *** HMAC Verification Failed!
  Sensor ID: 9999
  Sensor Type: Temperature
  Sensor Name: Sensor_Temperature_9999
  Value: 26.34 °C
  Timestamp: 1732291842
  Received HMAC: a3f2b8c1d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1

```

### 방법 2: 패킷 데이터 직접 변조 (고급)

코드를 직접 수정하여 HMAC 계산 후 데이터를 변경:

```c
// sensor_client.c 수정 예시
init_sensor_packet(&packet, sensor_id, sensor_type);

// HMAC 계산 후 데이터 변조
packet.value += 10.0;  // 온도 값 변조

send(sock, &packet, sizeof(packet), 0);
```

이 경우 서버에서 HMAC 검증이 실패하여 경고가 발생합니다.

## 로그 확인

### sensor_log.txt (전체 센서 로그)

모든 수신된 센서 데이터가 기록됩니다.

```bash
# 실시간으로 로그 확인
tail -f sensor_log.txt

# 로그 전체 보기
cat sensor_log.txt

# 특정 센서 ID만 필터링
grep "Sensor ID: 1001" sensor_log.txt

# HMAC 검증 실패만 보기
grep "INVALID" sensor_log.txt
```

**로그 예시:**
```
[Fri Nov 22 16:20:15 2024] Sensor ID: 1001, Type: Temperature, Value: 26.34 °C, HMAC: VALID
[Fri Nov 22 16:20:17 2024] Sensor ID: 1001, Type: Temperature, Value: 24.89 °C, HMAC: VALID
[Fri Nov 22 16:20:19 2024] Sensor ID: 1002, Type: Humidity, Value: 65.23 %, HMAC: VALID
[Fri Nov 22 16:20:21 2024] Sensor ID: 1003, Type: Pressure, Value: 1015.67 hPa, HMAC: VALID
```

### alert_log.txt (위변조 경고 로그)

HMAC 검증 실패 시에만 기록됩니다.

```bash
# 경고 로그 확인
cat alert_log.txt

# 실시간 모니터링
tail -f alert_log.txt
```

**경고 로그 예시:**
```
[Fri Nov 22 16:30:42 2024] *** ALERT *** HMAC Verification Failed!
  Sensor ID: 9999
  Sensor Type: Temperature
  Sensor Name: Sensor_Temperature_9999
  Value: 26.34 °C
  Timestamp: 1732291842
  Received HMAC: a3f2b8c1d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1

```

### 로그 파일 관리

```bash
# 로그 파일만 삭제
make clean-logs

# 로그 백업
cp sensor_log.txt sensor_log_$(date +%Y%m%d_%H%M%S).txt
cp alert_log.txt alert_log_$(date +%Y%m%d_%H%M%S).txt

# 로그 파일 크기 확인
du -h sensor_log.txt alert_log.txt
```

## 문제 해결

### 1. 컴파일 오류: "openssl/hmac.h: No such file or directory"

**원인:** OpenSSL 라이브러리가 설치되지 않음

**해결:**
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel

# macOS
brew install openssl
```

### 2. 실행 오류: "Connection refused"

**원인:** 서버가 실행되지 않음

**해결:**
1. 서버가 실행 중인지 확인
```bash
ps aux | grep monitor_server
```

2. 포트가 사용 중인지 확인
```bash
netstat -tuln | grep 8888
# 또는
lsof -i :8888
```

3. 서버를 먼저 실행한 후 클라이언트 실행

### 3. 실행 오류: "Bind failed: Address already in use"

**원인:** 포트 8888이 이미 사용 중

**해결:**
```bash
# 포트를 사용 중인 프로세스 확인
sudo lsof -i :8888

# 프로세스 종료
kill <PID>

# 또는 protocol.h에서 포트 변경
#define SERVER_PORT 9999  // 다른 포트로 변경
```

### 4. HMAC 검증 계속 실패

**원인:** 클라이언트와 서버의 SECRET_KEY가 다름

**해결:**
1. protocol.h의 SECRET_KEY 확인
2. make clean 후 전체 재빌드
```bash
make clean
make all
```

### 5. 서버가 종료되지 않음

**원인:** 무한 루프로 실행 중

**해결:**
```bash
# Ctrl+C 누르기
# 또는 다른 터미널에서
pkill monitor_server
```

### 6. Permission denied

**원인:** 실행 권한 없음

**해결:**
```bash
chmod +x monitor_server sensor_client
```

## 코드 구조 설명

### 1. protocol.h - 프로토콜 정의

```c
// 센서 데이터 패킷 구조체
typedef struct {
    uint32_t sensor_id;              // 센서 고유 ID
    SensorType sensor_type;          // 센서 타입 (1-4)
    float value;                     // 센서 측정값
    time_t timestamp;                // 측정 시각
    char sensor_name[MAX_SENSOR_NAME]; // 센서 이름
    uint8_t hmac[HMAC_SIZE];         // HMAC-SHA256 (32바이트)
} SensorPacket;
```

**설계 포인트:**
- HMAC은 패킷의 마지막에 위치
- HMAC 계산 시 HMAC 필드를 제외한 나머지 데이터 사용
- 타임스탬프로 재전송 공격(Replay Attack) 감지 가능

### 2. crypto_utils.c - HMAC 구현

**calculate_hmac() 함수:**
```c
int calculate_hmac(const SensorPacket *packet, uint8_t *hmac_out) {
    // HMAC 필드를 제외한 데이터 크기 계산
    size_t data_size = sizeof(SensorPacket) - HMAC_SIZE;

    // OpenSSL HMAC 함수 사용
    HMAC(EVP_sha256(),           // SHA-256 해시 알고리즘
         SECRET_KEY,             // 공유 비밀 키
         strlen(SECRET_KEY),     // 키 길이
         (unsigned char*)packet, // 데이터
         data_size,              // 데이터 길이
         hmac_out,               // 출력 버퍼
         &hmac_len);             // 출력 길이
}
```

**verify_hmac() 함수:**
```c
int verify_hmac(const SensorPacket *packet) {
    uint8_t calculated_hmac[HMAC_SIZE];

    // HMAC 재계산
    calculate_hmac(packet, calculated_hmac);

    // 수신된 HMAC과 비교
    return memcmp(calculated_hmac, packet->hmac, HMAC_SIZE);
}
```

### 3. sensor_utils.c - 센서 시뮬레이션

**generate_sensor_data() 함수:**
각 센서 타입에 맞는 랜덤 값 생성

**init_sensor_packet() 함수:**
1. 센서 데이터 생성
2. 타임스탬프 기록
3. HMAC 계산
4. 패킷 구성

### 4. sensor_client.c - 클라이언트

**주요 동작:**
1. TCP 소켓 생성
2. 서버 연결 (127.0.0.1:8888)
3. 10회 반복:
   - 센서 패킷 생성
   - 데이터 전송
   - 2초 대기
4. 연결 종료

### 5. monitor_server.c - 서버

**주요 동작:**
1. TCP 서버 소켓 생성 및 바인드
2. 포트 8888에서 리슨
3. 클라이언트 연결 수락
4. 데이터 수신 루프:
   - 패킷 수신
   - HMAC 검증
   - 결과 출력 및 로그 기록
   - 위변조 감지 시 경고
5. 연결 종료 처리

## 보안 메커니즘

### HMAC-SHA256

**특징:**
- **키 기반**: SECRET_KEY를 알아야만 올바른 HMAC 생성 가능
- **충돌 저항성**: 같은 HMAC을 가진 다른 데이터 생성 어려움
- **일방향성**: HMAC으로부터 원본 데이터 복원 불가능
- **무결성 보장**: 데이터의 1비트만 변경되어도 HMAC 완전히 변경

**보안 수준:**
- 출력 크기: 256비트 (32바이트)
- 보안 강도: 128비트 (SHA-256 기준)

### 공격 시나리오 및 방어

| 공격 유형 | 설명 | 방어 메커니즘 |
|---------|------|--------------|
| 데이터 변조 | 전송 중 센서값 변경 | HMAC 불일치로 탐지 |
| 중간자 공격 | 패킷 가로채기 및 변조 | HMAC 재계산 필요 (키 모름) |
| 재전송 공격 | 이전 패킷 재전송 | 타임스탬프 확인 가능 |
| 위조 센서 | 가짜 센서 데이터 전송 | SECRET_KEY 없으면 HMAC 생성 불가 |

## Makefile 타겟

```bash
make              # 전체 빌드
make clean        # 빌드 산출물 및 로그 삭제
make clean-logs   # 로그 파일만 삭제
make run-server   # 서버 빌드 및 실행
make run-client   # 클라이언트 빌드 및 실행 (기본 설정)
make demo         # 데모 실행 방법 안내
make help         # 도움말 표시
```

## 개발 정보

- **언어**: C (C11 표준)
- **암호화 라이브러리**: OpenSSL 1.1.1 이상
- **네트워크**: TCP/IP 소켓 (POSIX)
- **컴파일러**: GCC 4.8 이상
- **컴파일 옵션**:
  - `-Wall -Wextra`: 모든 경고 활성화
  - `-O2`: 최적화 레벨 2
  - `-std=c11`: C11 표준 준수
  - `-lssl -lcrypto`: OpenSSL 라이브러리 링크

## 라이선스

Educational/Research purposes only.

## 기여 및 문의

이 프로젝트는 암호화프로그래밍 수업의 기말 프로젝트로 제작되었습니다.
