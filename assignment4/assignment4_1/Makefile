obj-m := file_varea.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
	gcc -o test test.c

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f test
