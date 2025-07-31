# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -D_REENTRANT -pthread -I/usr/include/zookeeper
LDFLAGS = -lprotobuf-c -lpthread -lzookeeper_mt

# Directories
INCLUDE_DIR = ./include
SOURCE_DIR = ./source
OBJ_DIR = ./object
BIN_DIR = ./binary
PROTO_DIR = .

# Source files
CLIENT_SRCS = client_hashtable.c client_stub.c client_network.c message.c htmessages.pb-c.c
SERVER_SRCS = server_hashtable.c server_skeleton.c server_network.c message.c htmessages.pb-c.c 
COMMON_SRCS = table.c list.c entry.c block.c 

# Object files
CLIENT_OBJS = $(CLIENT_SRCS:%.c=$(OBJ_DIR)/%.o)
SERVER_OBJS = $(SERVER_SRCS:%.c=$(OBJ_DIR)/%.o)
COMMON_OBJS = $(COMMON_SRCS:%.c=$(OBJ_DIR)/%.o)

# Targets
CLIENT_TARGET = $(BIN_DIR)/client_hashtable
SERVER_TARGET = $(BIN_DIR)/server_hashtable

# Default target
all: directories proto $(CLIENT_TARGET) $(SERVER_TARGET)

# Create necessary directories if they don't exist
directories:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# Generate htmessages.pb-c.c and htmessages.pb-c.h from htmessages.proto
proto: $(PROTO_DIR)/htmessages.proto
	protoc-c --proto_path=$(PROTO_DIR) --c_out=$(SOURCE_DIR) $(PROTO_DIR)/htmessages.proto
	mv $(SOURCE_DIR)/htmessages.pb-c.h $(INCLUDE_DIR)

# Client target
$(CLIENT_TARGET): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) -o $@ $(CLIENT_OBJS) $(COMMON_OBJS) $(LDFLAGS)

# Server target
$(SERVER_TARGET): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) -o $@ $(SERVER_OBJS) $(COMMON_OBJS) $(LDFLAGS)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -I$(PROTO_DIR) -c $< -o $@

# Clean target to remove all generated files
clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/client_hashtable $(BIN_DIR)/server_hashtable $(SOURCE_DIR)/htmessages.pb-c.c $(INCLUDE_DIR)/htmessages.pb-c.h
