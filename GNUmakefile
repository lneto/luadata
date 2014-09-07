CC=gcc
CFLAGS=-I. -fPIC
LDLIBS=-llua
OBJ=luadata.o data.o handle.o layout.o binary.o luautil.o

data.so: $(OBJ)
	$(CC) -shared -o data.so $(OBJ) $(LDLIBS)

test: test.c data.so

clean:
	rm -f *.so *.o test || true
