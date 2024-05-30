ALL:client server
client:client.cc
	g++ -o $@ $^ -std=c++17;
server:TcpServer.cc
	g++ -g -o $@ $^ -std=c++17 -lpthread;

.PHONY:clean
clean:
	rm client server
