LUA_MODULES=	data

LUA_SRCS.data=	luadata.c
LUA_SRCS.data+=	data.c
LUA_SRCS.data+=	layout.c
LUA_SRCS.data+=	luautil.c
LUA_SRCS.data+=	binary.c

.include <bsd.lua.mk>
