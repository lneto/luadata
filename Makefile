all:
	gcc -o lbuf.so lbuf.c -std=c99 -llua -shared -fPIC
