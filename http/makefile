.PHONY:all
all: server client

server:main.cc
	g++ -o $@ $^ -lpthread
client:client1.cc
	g++ -o $@ $^ -lpthread

.PHONY:clean
clean:
	rm -f server client
