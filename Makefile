# Makefile for Sensor Monitoring System

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lssl -lcrypto

# Target executables
SERVER = monitor_server
CLIENT = sensor_client

# Object files
SERVER_OBJS = monitor_server.o crypto_utils.o sensor_utils.o
CLIENT_OBJS = sensor_client.o crypto_utils.o sensor_utils.o

# Default target
all: $(SERVER) $(CLIENT)

# Build server
$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build client
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile object files
%.o: %.c protocol.h crypto_utils.h sensor_utils.h
	$(CC) $(CFLAGS) -c $<

# Clean build artifacts
clean:
	rm -f $(SERVER) $(CLIENT) *.o
	rm -f sensor_log.txt alert_log.txt

# Clean only logs
clean-logs:
	rm -f sensor_log.txt alert_log.txt

# Run server
run-server: $(SERVER)
	./$(SERVER)

# Run client with default settings
run-client: $(CLIENT)
	./$(CLIENT)

# Run multiple clients with different sensor types
demo: $(SERVER) $(CLIENT)
	@echo "Starting demo..."
	@echo "Run './$(SERVER)' in one terminal"
	@echo "Run './$(CLIENT) 1001 1' for Temperature sensor"
	@echo "Run './$(CLIENT) 1002 2' for Humidity sensor"
	@echo "Run './$(CLIENT) 1003 3' for Pressure sensor"
	@echo "Run './$(CLIENT) 1004 4' for Vibration sensor"

# Help
help:
	@echo "Available targets:"
	@echo "  all          - Build both server and client (default)"
	@echo "  server       - Build monitor server only"
	@echo "  client       - Build sensor client only"
	@echo "  clean        - Remove all build artifacts and logs"
	@echo "  clean-logs   - Remove log files only"
	@echo "  run-server   - Build and run server"
	@echo "  run-client   - Build and run client"
	@echo "  demo         - Show demo instructions"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make"
	@echo "  make clean"
	@echo "  make run-server"
	@echo "  ./$(CLIENT) <sensor_id> <sensor_type>"
	@echo "    sensor_type: 1=Temperature, 2=Humidity, 3=Pressure, 4=Vibration"

.PHONY: all clean clean-logs run-server run-client demo help
