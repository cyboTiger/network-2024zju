all: server client

obj = server client
CFLAG = -lglog -lpthread -std=c++17

server: server.cpp server.hpp utils.hpp 
	g++ server.cpp -o server $(CFLAG)

client: client.cpp client.hpp utils.hpp
	g++ client.cpp -o client $(CFLAG)

clean:
	rm $(obj)

.PHONY: clean all
