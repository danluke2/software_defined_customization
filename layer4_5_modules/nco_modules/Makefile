
KBUILD_EXTRA_SYMBOLS := builder_arg
MODULE_DIR = builder_arg
BUILD_MODULE = builder_arg

obj-m += $(BUILD_MODULE)


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) clean
