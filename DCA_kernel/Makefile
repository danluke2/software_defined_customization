layer4_5-objs := init.o \
		       tap.o \
		       customization_socket.o \
					 register_cust.o \
					 send_recv_manager.o \
		       util/printing.o \
					 util/compare.o \
					 util/json-maker.o

obj-m += layer4_5.o

KCFLAGS = -Wall -O3 -g

# define more  debug values to print more to trace file
# NOTE: each DEBUG definition is independent of the others
# DEBUG will provide most comments which I find useful for customized flows
# DEBUG1 has extra comments for customized flows that might be useful
# DEBUG2 has very situational comments that can just clog up the file
# DEBUG3 most amount of comments
DEBUG_FLAGS = -DDEBUG 

all: layer4_5


layer4_5:
	make "KCFLAGS=$(KCFLAGS) $(DEBUG_FLAGS)" -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules


clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf *.o *.so

PREFIX = /usr/lib/modules/$(shell uname -r)/layer4_5

.PHONY: install-addons
install-addons: addons
	mkdir -p $(PREFIX)
	for FILE in $(INSTALL_FILES); do \
		mkdir -p "`dirname "$(PREFIX)/$$FILE"`"; \
		cp $$FILE $(PREFIX)/$$FILE; \
	done

.PHONY: install-layer4_5
install-layer4_5: layer4_5
	mkdir -p $(PREFIX)
	cp layer4_5.ko $(PREFIX)/
	cp Module.symvers $(PREFIX)/
	cp modules.order $(PREFIX)/



.PHONY: install
install: all install-layer4_5
