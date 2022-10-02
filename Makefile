CC = g++ -Wno-write-strings
CLIENT_FILE = simple_client.cpp
SERVER_FILE = simple_server.cpp
HTTP_SERVER_FILE = http_server.cpp

all: client server

client: $(CLIENT_FILE)
	$(CC) $(CLIENT_FILE) -lpthread -g -o client

server: $(SERVER_FILE) $(HTTP_SERVER_FILE)
	$(CC) $(SERVER_FILE) $(HTTP_SERVER_FILE) -lpthread -g -o server

clean:
	rm -f client server