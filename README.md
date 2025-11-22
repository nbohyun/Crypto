# 센서 모니터링 시스템

HMAC-SHA256 기반의 안전한 센서 데이터 전송 및 모니터링 시스템입니다.

## 주요 기능

- **센서 클라이언트**: 센서 데이터 생성, HMAC-SHA256 계산, TCP 전송
- **모니터링 서버**: TCP 수신, HMAC 검증, 로그 저장
- **4가지 센서 타입**: 온도, 습도, 기압, 진동
- **위변조 탐지**: HMAC 검증 실패 시 경고 및 별도 로그 기록
- **실시간 모니터링**: 센서 데이터 실시간 출력 및 로그 저장

## 파일 구조

```
Crypto/
├── protocol.h          # 프로토콜 구조체 정의
├── crypto_utils.h      # HMAC 함수 헤더
├── crypto_utils.c      # HMAC 함수 구현
├── sensor_utils.h      # 센서 시뮬레이션 헤더
├── sensor_utils.c      # 센서 시뮬레이션 구현
├── sensor_client.c     # 센서 클라이언트 메인
├── monitor_server.c    # 모니터링 서버 메인
├── Makefile           # 빌드 설정
└── README.md          # 사용 설명서
```

## 요구 사항

- GCC 컴파일러
- OpenSSL 라이브러리 (libssl-dev)

### Ubuntu/Debian에서 설치:
```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

## 빌드

```bash
# 전체 빌드
make

# 클린 빌드
make clean
make all
```

## 실행 방법

### 1. 서버 실행

터미널 1에서:
```bash
./monitor_server
```

서버는 포트 8888에서 연결을 대기합니다.

### 2. 클라이언트 실행

터미널 2, 3, 4, 5에서 각각 다른 센서 타입으로 실행:

```bash
# 온도 센서 (센서 ID: 1001, 타입: 1)
./sensor_client 1001 1

# 습도 센서 (센서 ID: 1002, 타입: 2)
./sensor_client 1002 2

# 기압 센서 (센서 ID: 1003, 타입: 3)
./sensor_client 1003 3

# 진동 센서 (센서 ID: 1004, 타입: 4)
./sensor_client 1004 4
```

각 클라이언트는 10개의 센서 데이터를 2초 간격으로 전송합니다.

## 센서 타입

| 타입 번호 | 센서 타입 | 데이터 범위 | 단위 |
|----------|----------|------------|------|
| 1 | Temperature (온도) | 20~30 | °C |
| 2 | Humidity (습도) | 40~80 | % |
| 3 | Pressure (기압) | 980~1040 | hPa |
| 4 | Vibration (진동) | 0~100 | Hz |

## 로그 파일

- **sensor_log.txt**: 모든 수신된 센서 데이터 기록
- **alert_log.txt**: HMAC 검증 실패 시 경고 로그

로그 파일 삭제:
```bash
make clean-logs
```

## HMAC 보안

- **알고리즘**: HMAC-SHA256
- **키**: `shared_secret_key_2024` (protocol.h에서 정의)
- **검증**: 서버에서 수신된 모든 패킷의 HMAC 검증
- **위변조 탐지**: HMAC 불일치 시 경고 및 별도 로그 기록

## 출력 예시

### 서버 출력:
```
Sensor Monitoring Server
Port: 8888
Log file: sensor_log.txt
Alert file: alert_log.txt

Server is listening on port 8888...

New connection from 127.0.0.1:45678
[Fri Nov 22 16:20:15 2024] Sensor: Temperature (ID: 1001) | Value: 26.34 °C | HMAC: ✓ VALID
[Fri Nov 22 16:20:17 2024] Sensor: Temperature (ID: 1001) | Value: 24.89 °C | HMAC: ✓ VALID
```

### 클라이언트 출력:
```
Sensor Client Started
Sensor ID: 1001
Sensor Type: Temperature
Target Server: 127.0.0.1:8888

Connected to server

[1] Sending data: Temperature = 26.34 °C (HMAC: a3f2b8c1d4e5f6a7...)
[2] Sending data: Temperature = 24.89 °C (HMAC: b4c3d2e1f0a9b8c7...)
```

## Makefile 타겟

```bash
make              # 전체 빌드
make clean        # 빌드 산출물 및 로그 삭제
make clean-logs   # 로그 파일만 삭제
make run-server   # 서버 빌드 및 실행
make run-client   # 클라이언트 빌드 및 실행
make demo         # 데모 실행 방법 안내
make help         # 도움말 표시
```

## 개발 정보

- **언어**: C (C11 표준)
- **암호화 라이브러리**: OpenSSL
- **네트워크**: TCP/IP 소켓
- **컴파일 옵션**: `-Wall -Wextra -O2 -std=c11`

## 라이선스

Educational/Research purposes only.
