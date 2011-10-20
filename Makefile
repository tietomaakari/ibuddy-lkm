
obj-m += ibuddy.o
ibuddy-objs := main.o usbcomm.o attributes.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	@rm -f *~
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

