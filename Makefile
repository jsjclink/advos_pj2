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
	$(CC) $(CFLAGS) src/tiny_file_client.c $(OUT) -o bin/usapp
	$(CC) $(CFLAGS) src/tiny_file_server.c $(OUT) -o bin/service -L ./snappy-c -lsnappyc
	@export LD_LIBRARY_PATH=./snappy-c
clean :
	@rm src/*.o bin/*.a bin/usapp bin/service
	@echo Cleaned!