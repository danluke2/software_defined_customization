
KBUILD_EXTRA_SYMBOLS := builder_arg
MODULE_DIR = builder_arg
module = builder_arg

obj-m += $(module)


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) clean
