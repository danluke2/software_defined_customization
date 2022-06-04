# ************** STANDARD PARAMS MUST GO HERE ****************
KBUILD_EXTRA_SYMBOLS=/lib/modules/5.13.0-35-generic/layer4_5/Module.symvers
MODULE_DIR=/home/vagrant/software_defined_customization/layer4_5_modules/sample_modules
DISTRO_DIR=/lib/modules/5.13.0-35-generic
# ************** END STANDARD PARAMS ****************


#this is the module you want to create (ex: sample_python_client.o)
BUILD_MODULE = must_pass_command_arg

obj-m += $(BUILD_MODULE)


all:
	make -C $(DISTRO_DIR)/build M=$(MODULE_DIR) modules

clean:
	make -C $(DISTRO_DIR)/build M=$(MODULE_DIR) clean
