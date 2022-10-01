CC = gcc
CLIENT_FILE = simple_client.cpp
SERVER_FILE = simple_server.cpp

all: client server

client: $(CLIENT_FILE)
	$(CC) $(CLIENT_FILE) -lpthread -o client

server: $(SERVER_FILE)
	$(CC) $(SERVER_FILE) -lpthread -o server

clean:
	rm -f client server