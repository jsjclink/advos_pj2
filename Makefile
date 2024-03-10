CC = gcc
CFLAGS = -g 
LDFLAGS = 
LIBS = .
SRC = src/tiny_file.c
OBJ = $(SRC:.c=.o)

OUT = bin/libtf.a

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

app:
	$(CC) $(CFLAGS) src/tiny_file_client.c $(OUT) -o bin/usapp -pthread
	$(CC) $(CFLAGS) src/tiny_file_client2.c $(OUT) -o bin/usapp2 -pthread
	$(CC) $(CFLAGS) src/tiny_file_server.c -o bin/service -L ./ -lsnappyc
lib : snappy.o libsnappyc.so
snappy.o: snappy.c compat.h snappy-int.h
libsnappyc.so: snappy.o
	$(CC) $(LDFLAGS) -shared -o $@ $^   
clean :
	@rm src/*.o bin/*.a bin/usapp bin/usapp2 bin/service snappy.o libsnappyc.so
	@echo Cleaned!
