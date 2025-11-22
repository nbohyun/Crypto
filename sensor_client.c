#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "protocol.h"
#include "sensor_utils.h"

#define SERVER_PORT 8888
#define SERVER_IP "127.0.0.1"

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    SensorPacket packet;
    uint32_t sensor_id = 1001;
    SensorType sensor_type = SENSOR_TEMPERATURE;

    // 명령줄 인자 처리
    if (argc >= 2) {
        sensor_id = atoi(argv[1]);
    }
    if (argc >= 3) {
        sensor_type = (SensorType)atoi(argv[2]);
        if (sensor_type < SENSOR_TEMPERATURE || sensor_type > SENSOR_VIBRATION) {
            printf("Invalid sensor type. Using default (Temperature)\n");
            sensor_type = SENSOR_TEMPERATURE;
        }
    }

    // 랜덤 시드 초기화
    srand(time(NULL) + sensor_id);

    printf("Sensor Client Started\n");
    printf("Sensor ID: %d\n", sensor_id);
    printf("Sensor Type: %s\n", get_sensor_type_name(sensor_type));
    printf("Target Server: %s:%d\n\n", SERVER_IP, SERVER_PORT);

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        exit(1);
    }

    // 서버 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(1);
    }

    printf("Connected to server\n\n");

    // 센서 데이터 전송 루프
    for (int i = 0; i < 10; i++) {
        // 센서 패킷 생성
        init_sensor_packet(&packet, sensor_id, sensor_type);

        printf("[%d] Sending data: %s = %.2f %s (HMAC: ",
               i + 1,
               get_sensor_type_name(sensor_type),
               packet.value,
               get_sensor_unit(sensor_type));

        // HMAC 출력
        for (int j = 0; j < 8; j++) {
            printf("%02x", packet.hmac[j]);
        }
        printf("...)\n");

        // 데이터 전송
        ssize_t sent = send(sock, &packet, sizeof(packet), 0);
        if (sent < 0) {
            perror("Send failed");
            break;
        }

        // 2초 대기
        sleep(2);
    }

    printf("\nTransmission completed. Closing connection.\n");
    close(sock);

    return 0;
}
