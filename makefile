LDFLAGS+=-pthread
all : main client 
main : main.o media_lib.o pool.o queue.o 
	gcc -o $@ $^ $(LDFLAGS)
client : client.o
	gcc -o $@ $^
clean :
	rm -rf *.o main client
