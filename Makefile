obj-m := hw2.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

# cmd for testing. comment below code before submit

# i: # usage: make p=5 i
# ifdef p
# 	sudo insmod hw2.ko period=$(p)
# else
# 	sudo insmod hw2.ko period=5
# endif

# r:
# 	sudo rmmod hw2.ko