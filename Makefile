all: http-server

http-server: src/*.c
	gcc -Wall -g src/*.c -o http-server -lssl -lcrypto -lpthread -Iinclude

clean:
	@rm http-server
