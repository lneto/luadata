#TODO: implement decent makefile!
all:
	gcc -o bitwiser.so bitwiser.c lua_bitwiser.c bit_util.c lua_util.c -std=c99 -llua -shared -fPIC
