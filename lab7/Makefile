all: server client

obj = server client a b

trial: a b

CFLAG = -lglog -lpthread -std=c++17

server: server.cpp server.hpp utils.hpp 
	g++ server.cpp -o server $(CFLAG)

client: client.cpp client.hpp utils.hpp
	g++ client.cpp -o client $(CFLAG)

a: a.cpp utils.hpp
	g++ a.cpp -o a $(CFLAG)

b: b.cpp utils.hpp
	g++ b.cpp -o b $(CFLAG)

clean:
	rm $(obj)

.PHONY: clean all
