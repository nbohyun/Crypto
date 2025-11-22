#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <time.h>

#define HMAC_SIZE 32
#define MAX_SENSOR_NAME 32
#define SECRET_KEY "shared_secret_key_2024"

// 센서 타입 정의
typedef enum {
    SENSOR_TEMPERATURE = 1,
    SENSOR_HUMIDITY = 2,
    SENSOR_PRESSURE = 3,
    SENSOR_VIBRATION = 4
} SensorType;

// 센서 데이터 패킷 구조체
typedef struct {
    uint32_t sensor_id;
    SensorType sensor_type;
    float value;
    time_t timestamp;
    char sensor_name[MAX_SENSOR_NAME];
    uint8_t hmac[HMAC_SIZE];
} SensorPacket;

// 함수 선언
const char* get_sensor_type_name(SensorType type);

#endif // PROTOCOL_H
