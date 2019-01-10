ccflags-y += -D_KERNEL -D'CHAR_BIT=(8)' \
	-D'MIN=min' -D'MAX=max' -D'UCHAR_MAX=(255)' -D'UINT64_MAX=((u64)~0ULL)'

obj-$(CONFIG_LUADATA) += luadata.o
luadata-objs += binary.o data.o handle.o layout.o luadata_core.o luautil.o
