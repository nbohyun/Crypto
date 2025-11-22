#include "sensor_utils.h"
#include "crypto_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 센서 타입 이름 반환
const char* get_sensor_type_name(SensorType type) {
    switch (type) {
        case SENSOR_TEMPERATURE:
            return "Temperature";
        case SENSOR_HUMIDITY:
            return "Humidity";
        case SENSOR_PRESSURE:
            return "Pressure";
        case SENSOR_VIBRATION:
            return "Vibration";
        default:
            return "Unknown";
    }
}

// 센서 단위 반환
const char* get_sensor_unit(SensorType type) {
    switch (type) {
        case SENSOR_TEMPERATURE:
            return "°C";
        case SENSOR_HUMIDITY:
            return "%";
        case SENSOR_PRESSURE:
            return "hPa";
        case SENSOR_VIBRATION:
            return "Hz";
        default:
            return "";
    }
}

// 센서 데이터 생성 (시뮬레이션)
float generate_sensor_data(SensorType type) {
    float base_value;
    float variation;

    switch (type) {
        case SENSOR_TEMPERATURE:
            // 온도: 20~30°C 범위
            base_value = 25.0;
            variation = ((float)rand() / RAND_MAX) * 10.0 - 5.0;
            break;
        case SENSOR_HUMIDITY:
            // 습도: 40~80% 범위
            base_value = 60.0;
            variation = ((float)rand() / RAND_MAX) * 40.0 - 20.0;
            break;
        case SENSOR_PRESSURE:
            // 기압: 980~1040 hPa 범위
            base_value = 1013.25;
            variation = ((float)rand() / RAND_MAX) * 60.0 - 30.0;
            break;
        case SENSOR_VIBRATION:
            // 진동: 0~100 Hz 범위
            base_value = 50.0;
            variation = ((float)rand() / RAND_MAX) * 100.0 - 50.0;
            break;
        default:
            base_value = 0.0;
            variation = 0.0;
    }

    return base_value + variation;
}

// 센서 패킷 초기화
void init_sensor_packet(SensorPacket *packet, uint32_t sensor_id, SensorType type) {
    memset(packet, 0, sizeof(SensorPacket));

    packet->sensor_id = sensor_id;
    packet->sensor_type = type;
    packet->value = generate_sensor_data(type);
    packet->timestamp = time(NULL);

    snprintf(packet->sensor_name, MAX_SENSOR_NAME, "Sensor_%s_%d",
             get_sensor_type_name(type), sensor_id);

    // HMAC 계산
    calculate_hmac(packet, packet->hmac);
}
