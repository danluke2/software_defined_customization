# ************** STANDARD PARAMS MUST GO HERE ****************
distro=$(shell uname -r)
INSTALLER_MAKEFILE_DIR=/home/vagrant/software_defined_customization/DCA_kernel
INSTALL_LOCATION=/usr/lib/modules/$(distro)/layer4_5
DISTRO_DIR=/lib/modules/$(distro)
# ************** END STANDARD PARAMS ****************


layer4_5-objs := init.o \
		       tap.o \
		       customization_socket.o \
					 register_cust.o \
					 send_recv_manager.o \
		       util/helpers.o \
					 util/compare.o \
					 util/json-maker.o

obj-m += layer4_5.o

KCFLAGS = -Wall -O3 -g

#TODO: Need DEBUG documentation and a clear purpose throughout

# define more  debug values to print more to trace file
# NOTE: each DEBUG definition is independent of the others
# DEBUG will provide most comments which I find useful for customized flows
# DEBUG1 has extra comments for customized flows that might be useful
# DEBUG2 has very situational comments that can just clog up the file
# DEBUG3 most amount of comments
# DEBUG_FLAGS = -DDEBUG -DDEBUG1 -DDEBUG2 -DDEBUG3
DEBUG_FLAGS = -DDEBUG 

all: layer4_5


layer4_5:
	make "KCFLAGS=$(KCFLAGS) $(DEBUG_FLAGS)" -C $(DISTRO_DIR)/build M=$(INSTALLER_MAKEFILE_DIR) modules


clean:
	make -C $(DISTRO_DIR)/build M=$(INSTALLER_MAKEFILE_DIR) clean
	rm -rf *.o *.so



.PHONY: install-layer4_5
install-layer4_5: layer4_5
	mkdir -p $(INSTALL_LOCATION)
	cp layer4_5.ko $(INSTALL_LOCATION)/
	cp Module.symvers $(INSTALL_LOCATION)/
	cp modules.order $(INSTALL_LOCATION)/



.PHONY: install
install: all install-layer4_5
