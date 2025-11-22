#ifndef SENSOR_UTILS_H
#define SENSOR_UTILS_H

#include "protocol.h"

// 센서 데이터 생성 함수
float generate_sensor_data(SensorType type);

// 센서 패킷 초기화 함수
void init_sensor_packet(SensorPacket *packet, uint32_t sensor_id, SensorType type);

// 센서 타입 이름 반환
const char* get_sensor_type_name(SensorType type);

// 센서 단위 반환
const char* get_sensor_unit(SensorType type);

#endif // SENSOR_UTILS_H
