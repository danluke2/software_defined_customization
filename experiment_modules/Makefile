
#set this location to where your software_defined_customization folder is located
GIT_LOCATION = /home/dan/software_defined_customization


KBUILD_EXTRA_SYMBOLS := /lib/modules/$(shell uname -r)/layer4_5/Module.symvers
#this is the directory of the makefile
MODULE_DIR = $(GIT_LOCATION)/experiment_modules
#this is the module you want to create (ex: sample_python_client.o)
BUILD_MODULE = must_pass_command_arg

obj-m += $(BUILD_MODULE)


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) clean
