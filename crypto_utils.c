#include "crypto_utils.h"
#include <string.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

// HMAC-SHA256 계산
int calculate_hmac(const SensorPacket *packet, uint8_t *hmac_out) {
    unsigned int hmac_len = HMAC_SIZE;

    // HMAC을 제외한 데이터 부분만 해시 계산
    size_t data_size = sizeof(SensorPacket) - HMAC_SIZE;

    // HMAC 계산 (OpenSSL 3.0+ 호환)
    unsigned char *result = HMAC(EVP_sha256(),
                                 SECRET_KEY,
                                 strlen(SECRET_KEY),
                                 (unsigned char*)packet,
                                 data_size,
                                 hmac_out,
                                 &hmac_len);

    if (result == NULL) {
        return -1;
    }

    return 0;
}

// HMAC 검증
int verify_hmac(const SensorPacket *packet) {
    uint8_t calculated_hmac[HMAC_SIZE];

    // HMAC 계산
    if (calculate_hmac(packet, calculated_hmac) != 0) {
        return -1;
    }

    // 계산된 HMAC과 패킷의 HMAC 비교
    if (memcmp(calculated_hmac, packet->hmac, HMAC_SIZE) != 0) {
        return -1; // 검증 실패
    }

    return 0; // 검증 성공
}
