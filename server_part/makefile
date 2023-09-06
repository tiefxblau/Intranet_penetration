server:server.cpp
	g++ -o $@ *.cpp -std=c++11 -ljsoncpp -lboost_system -lboost_date_time -lpthread

.PHONY:clean
clean:
	rm -f server

.PHONT:debug
debug:
	g++ -fdiagnostics-color=always -o ../build/server *.cpp -g -std=c++11 -ljsoncpp -lboost_system -lboost_date_time -lpthread