#TODO: implement decent makefile!
all:
	gcc -o lbuf.so lbuf.c bit_util.c -std=c99 -llua -shared -fPIC
