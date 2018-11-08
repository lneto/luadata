ccflags-y += -D_KERNEL -D'CHAR_BIT=(8)' \
	-D'MIN=min' -D'MAX=max' -D'UCHAR_MAX=(255)' -D'UINT64_MAX=((u64)~0ULL)'

obj-$(CONFIG_LUADATA) += kluadata.o
kluadata-objs += binary.o data.o handle.o layout.o luadata.o luautil.o
