

KBUILD_EXTRA_SYMBOLS := /home/dan/layer4.5/DCA_kernel/Module.symvers
MODULE_DIR = /home/dan/layer4.5/DCA_kernel/test_modules
BUILD_MODULE = must_pass_command_arg

obj-m += $(BUILD_MODULE)


all:
	# make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) modules

clean:
	# make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) clean
