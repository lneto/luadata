CC=gcc
CFLAGS=-I. -fPIC
LDLIBS=-llua
OBJ=luadata.o data.o layout.o binary.o luautil.o

data.so: $(OBJ)
	$(CC) -shared -o data.so $(OBJ)

test: test.c data.so

clean:
	rm *.so *.o
