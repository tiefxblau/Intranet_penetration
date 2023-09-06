client:client.cpp
	g++ -o $@ *.cpp -std=c++11 -ljsoncpp -lboost_system -lboost_date_time -lpthread

.PHONY:clean
clean:
	rm -f client

.PHONT:debug
debug:
	g++ -fdiagnostics-color=always -o ../build/client *.cpp -g -std=c++11 -ljsoncpp -lboost_system -lboost_date_time -lpthread