#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include "protocol.h"
#include "crypto_utils.h"
#include "sensor_utils.h"

#define SERVER_PORT 8888
#define MAX_CLIENTS 10
#define LOG_FILE "sensor_log.txt"
#define ALERT_FILE "alert_log.txt"

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    running = 0;
    printf("\nShutdown signal received. Closing server...\n");
}

void log_sensor_data(const SensorPacket *packet, int verified) {
    FILE *log_fp = fopen(LOG_FILE, "a");
    if (log_fp == NULL) {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    char time_str[26];
    ctime_r(&now, time_str);
    time_str[24] = '\0'; // Remove newline

    fprintf(log_fp, "[%s] Sensor ID: %u, Type: %s, Value: %.2f %s, HMAC: %s\n",
            time_str,
            packet->sensor_id,
            get_sensor_type_name(packet->sensor_type),
            packet->value,
            get_sensor_unit(packet->sensor_type),
            verified ? "VALID" : "INVALID");

    fclose(log_fp);
}

void log_alert(const SensorPacket *packet) {
    FILE *alert_fp = fopen(ALERT_FILE, "a");
    if (alert_fp == NULL) {
        perror("Failed to open alert file");
        return;
    }

    time_t now = time(NULL);
    char time_str[26];
    ctime_r(&now, time_str);
    time_str[24] = '\0'; // Remove newline

    fprintf(alert_fp, "[%s] *** ALERT *** HMAC Verification Failed!\n", time_str);
    fprintf(alert_fp, "  Sensor ID: %u\n", packet->sensor_id);
    fprintf(alert_fp, "  Sensor Type: %s\n", get_sensor_type_name(packet->sensor_type));
    fprintf(alert_fp, "  Sensor Name: %s\n", packet->sensor_name);
    fprintf(alert_fp, "  Value: %.2f %s\n", packet->value, get_sensor_unit(packet->sensor_type));
    fprintf(alert_fp, "  Timestamp: %ld\n", packet->timestamp);
    fprintf(alert_fp, "  Received HMAC: ");
    for (int i = 0; i < HMAC_SIZE; i++) {
        fprintf(alert_fp, "%02x", packet->hmac[i]);
    }
    fprintf(alert_fp, "\n\n");

    fclose(alert_fp);
}

void handle_client(int client_sock, struct sockaddr_in *client_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);

    printf("New connection from %s:%d\n", client_ip, ntohs(client_addr->sin_port));

    SensorPacket packet;
    ssize_t received;

    while (running) {
        // 데이터 수신
        received = recv(client_sock, &packet, sizeof(packet), 0);

        if (received <= 0) {
            if (received == 0) {
                printf("Client %s disconnected\n", client_ip);
            } else {
                perror("Receive error");
            }
            break;
        }

        if (received != sizeof(packet)) {
            printf("Warning: Received incomplete packet (%zd bytes)\n", received);
            continue;
        }

        // HMAC 검증
        int verified = (verify_hmac(&packet) == 0);

        // 로그 출력
        char time_str[26];
        ctime_r(&packet.timestamp, time_str);
        time_str[24] = '\0';

        printf("[%s] Sensor: %s (ID: %u) | Value: %.2f %s | HMAC: %s\n",
               time_str,
               get_sensor_type_name(packet.sensor_type),
               packet.sensor_id,
               packet.value,
               get_sensor_unit(packet.sensor_type),
               verified ? "✓ VALID" : "✗ INVALID");

        // 파일에 로그 기록
        log_sensor_data(&packet, verified);

        // 위변조 감지 시 경고
        if (!verified) {
            printf("*** WARNING: Data tampering detected! ***\n");
            log_alert(&packet);
        }
    }

    close(client_sock);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 시그널 핸들러 설정
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Sensor Monitoring Server\n");
    printf("Port: %d\n", SERVER_PORT);
    printf("Log file: %s\n", LOG_FILE);
    printf("Alert file: %s\n\n", ALERT_FILE);

    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // 소켓 옵션 설정 (주소 재사용)
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_sock);
        exit(1);
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // 바인드
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(1);
    }

    // 리슨
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(1);
    }

    printf("Server is listening on port %d...\n\n", SERVER_PORT);

    // 클라이언트 연결 처리
    while (running) {
        // 타임아웃 설정을 위한 select 사용
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(server_sock + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            if (running) {
                perror("Select error");
            }
            break;
        }

        if (activity == 0) {
            // 타임아웃, 계속 대기
            continue;
        }

        // 새 연결 수락
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            if (running) {
                perror("Accept failed");
            }
            continue;
        }

        // 클라이언트 처리 (간단한 구현: 단일 클라이언트)
        handle_client(client_sock, &client_addr);
    }

    close(server_sock);
    printf("Server shutdown complete.\n");

    return 0;
}
