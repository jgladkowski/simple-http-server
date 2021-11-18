CC = g++
CFLAGS = -std=c++17

all: http_parser.o serwer.o
	$(CC) $(CFLAGS) -o serwer http_parser.o serwer.o -lstdc++fs
serwer.o: serwer.cpp
	$(CC) $(CFLAGS) -c -o serwer.o serwer.cpp
http_parser.o: http_parser.cpp
	$(CC) $(CFLAGS) -c -o http_parser.o http_parser.cpp
clean:
	rm http_parser.o
	rm serwer.o
	rm serwer
