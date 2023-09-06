.PHONY:all
all: calc_server calc_client

calc_server:calc_server.cpp
	g++ -o $@ $^ -std=c++11 -lpthread -ljsoncpp

calc_client:calc_client.cpp
	g++ -o $@ $^ -std=c++11 -ljsoncpp

.PHONY:clean
clean:
	rm -f calc_client calc_server