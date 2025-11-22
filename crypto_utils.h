#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include "protocol.h"

// HMAC-SHA256 계산 함수
int calculate_hmac(const SensorPacket *packet, uint8_t *hmac_out);

// HMAC 검증 함수
int verify_hmac(const SensorPacket *packet);

#endif // CRYPTO_UTILS_H
