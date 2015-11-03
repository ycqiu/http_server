all : server 

server:  http_server.cpp
	g++ -Wall http_server.cpp -levent_core -o server 
