CFLAGS = -Wall  -std=c++11
CC = c++
#CC = clang++

DEPS = main.h  bytes_array.h  huffman.h

OBJS = http2_client.o \
	work_loop.o \
	socket.o \
	functions.o \
	ssl.o \

http2_client: $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lpthread -L/usr/local/lib/ -L/usr/local/lib64/ -lssl -lcrypto

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f http2_client
	rm -f *.o
