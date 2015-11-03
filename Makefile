all : server 

server:  http_server.cpp
	g++ -Wall $^ -levent_core -o $@ 

clean:
	rm -f server

.PHONY: clean
