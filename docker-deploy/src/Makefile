all: main

main: main.cpp helper.h proxy.cpp proxy.h request.cpp request.h
	g++ --std=c++11 -g  -o main main.cpp helper.h proxy.cpp proxy.h request.cpp request.h -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main *~ *#