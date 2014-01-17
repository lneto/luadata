CC=gcc
CFLAGS=-I. -fPIC
OBJ=luadata.o data.o layout.o binary.o luautil.o

data.so: $(OBJ)
	$(CC) -shared -o data.so $(OBJ)

clean:
	rm *.so *.o
