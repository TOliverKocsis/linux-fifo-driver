KDIR      ?= /lib/modules/$(shell uname -r)/build
BUILD_DIR := $(CURDIR)/build

.PHONY: all clean

all:
	mkdir -p $(BUILD_DIR)
	$(MAKE) -C $(KDIR) M=$(BUILD_DIR) src=$(CURDIR) modules

clean:
	rm -rf $(BUILD_DIR)
