obj-m += temp_driver.o display_driver.o
KDIR = /lib/modules/$(shell uname -r)/build

all: drivers app

app:
	gcc i2c_app.c -o app

drivers:
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	if [ -f ./app ]; then \
		rm -f ./app; \
	fi
