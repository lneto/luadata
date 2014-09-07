LUA_MODULES=	data

LUA_SRCS.data=	luadata.c
LUA_SRCS.data+=	data.c
LUA_SRCS.data+=	handle.c
LUA_SRCS.data+=	layout.c
LUA_SRCS.data+=	luautil.c
LUA_SRCS.data+=	binary.c

DATA=		data.so
LDLIBS= 	-llua  ${DATA}
test: 		test.c ${DATA}

CLEANFILES+= 	test

.include <bsd.lua.mk>
