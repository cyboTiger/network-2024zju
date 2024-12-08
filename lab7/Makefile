all: server client

obj = server client

server: server.cpp
	g++ server.cpp -o server -lglog -lpthread

client: client.cpp
	g++ client.cpp -o client -lglog -lpthread

clean:
	rm $(obj)

.PHONY: clean all
