PROJECTS=kernel
HEADER_PROJECTS=kernel

ROOT?=$(CURDIR)
SYSROOT=$(ROOT)/sysroot
ISODIR?=$(ROOT)/isodir
export DESTDIR?=$(SYSROOT)

# Default host is given by script
DEFAULT_HOST!=./default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=./target-triplet-to-arch.sh $(HOST)
# Grub Binary
GRUB_BINARY_NAME?=stage2_eltorito
GRUB_BINARY_PATH?=$(ROOT)/kernel/arch/$(HOSTARCH)
GRUB_BINARY=$(GRUB_BINARY_PATH)/$(GRUB_BINARY_NAME)

export CC:=$(HOST)-gcc --sysroot=$(SYSROOT)
export AR:=$(HOST)-ar

.PHONY: all
all: os_2.iso

os_2.iso: install-headers $(PROJECTS)
	mkdir -p $(ISODIR)/boot/grub
	cp $(ROOT)/kernel/os_2.kernel $(ISODIR)/boot
	cp $(GRUB_BINARY) $(ISODIR)/boot/grub
	cp $(ROOT)/grub_menu.lst $(ISODIR)/boot/grub/menu.lst
	@echo "Generating iso ..."
	@genisoimage -R                               \
	            -b boot/grub/$(GRUB_BINARY_NAME) \
				-no-emul-boot                    \
				-boot-load-size 4                \
				-A os                            \
				-input-charset utf8              \
				-quiet                           \
				-boot-info-table                 \
				-o $(ROOT)/os_2.iso              \
				$(ISODIR)
	@echo "Done"

.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) -C $(ROOT)/$@

.PHONY: clean
clean:
	rm -rf $(DESTDIR)
	rm -rf $(ISODIR)
	rm -f $(ROOT)/os_2.iso
	for dir in $(PROJECTS); do \
	  $(MAKE) clean -C $(ROOT)/$$dir; \
	done

.PHONY: install-headers
install-headers:
	for dir in $(PROJECTS); do \
	  $(MAKE) install-headers -C $(ROOT)/$$dir; \
	done
