BUILDDIR = ./build
OSNAME = MyOS

# Kernel directory
KERNELDIR = kernel
KERNELBIN = $(KERNELDIR)/bin/kernel.elf
KERNELDATA = $(wildcard $(KERNELDIR)/bin/data/*)

# Bootsector properties
LIMINEDIR = limine
LIMINEEFI = $(LIMINEDIR)/BOOTX64.EFI
LIMINECFG = limine.cfg
LIMINEDATA = $(wildcard limine_data/*)
STARTUPNSH = ./startup.nsh

# Image properties
IMAGESIZE = 10000 # The size of the OS image, in KiB

ISOROOT = isoroot

QEMU_FLAGS = -m 512M -cpu qemu64 -net none

.PHONY: buildimg
buildimg: 
	@mkdir -p build
	@echo "Creating an empty image..."
# Create a file with size 512kB containin zeros
	@dd if=/dev/zero of=$(BUILDDIR)/$(OSNAME).img bs=1024 count=$(IMAGESIZE) 2>/dev/null
# Format the file as a DOS/MBR bootsector
	@echo "Formating the image..."
	@echo "* Creating the filesystem"
	@mformat -i $(BUILDDIR)/$(OSNAME).img ::
	@echo "* Creating /EFI directory"
	@mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI
	@echo "* Creating /EFI/BOOT directory"
	@mmd -i $(BUILDDIR)/$(OSNAME).img ::/EFI/BOOT
	@echo "* Creating /KERNEL directory"
	@mmd -i $(BUILDDIR)/$(OSNAME).img ::/KERNEL
	@echo "* Creating /KERNEL/DATA directory"
	@mmd -i $(BUILDDIR)/$(OSNAME).img ::/KERNEL/DATA
# Copy all the data into it
	@echo "Copying bootloader into image..."
	@echo "* Copying bootloader binary into"
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(LIMINEEFI) ::/EFI/BOOT
	@echo "* Copying startup.nsh"
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(STARTUPNSH) ::
	@echo "* Copying Limine config" 
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(LIMINECFG) ::
	@echo "* Copying $(LIMINEDATA) into /EFI/BOOT"; 
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(LIMINEDATA) ::/EFI/BOOT; 

	@echo "Copying kernel.elf into image..."
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(KERNELDIR)/bin/kernel.elf ::KERNEL 
	@echo "Copying kernel ${KERNELDATA} into image..."
	@mcopy -i $(BUILDDIR)/$(OSNAME).img $(KERNELDATA) ::/KERNEL/DATA

$(ISOROOT): $(KERNELDATA) $(LIMINECFG) $(LIMINEDATA) $(LIMINEDIR) $(KERNELBIN)
	@mkdir -p $(ISOROOT)  $(ISOROOT)/bootloader/data  $(ISOROOT)/kernel/data
	@cp -v 	$(LIMINEDATA)  $(ISOROOT)/bootloader/data
	@cp -v 	$(LIMINECFG)   $(ISOROOT)/
	@cp -v 	$(LIMINEDIR)/limine.sys \
		$(LIMINEDIR)/limine-cd.bin \
		$(LIMINEDIR)/limine-cd-efi.bin  $(ISOROOT)/
	@cp -v 	$(KERNELBIN)  $(ISOROOT)/kernel/kernel.elf
	@cp -v 	$(KERNELDATA)  $(ISOROOT)/kernel/data
	@echo Created a temporary directory with the OS files.

.PHONY: buildiso
buildiso: $(ISOROOT)
# Create the ISO with the efi bootloader
	@echo Creating the ISO with the efi bootloader...
	xorriso -as mkisofs \
		-b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-cd-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISOROOT) -o $(OSNAME).iso

# Add the legacy bootloader
	@echo Adding legacy bootloader into the ISO...
	$(LIMINEDIR)/limine-deploy $(OSNAME).iso

	@echo Cleaning up...
	@rm -R $(ISOROOT)

	@echo Finished!


$(KERNELBIN):
	make -C kernel clean all

$(LIMINEDIR):
	git clone https://github.com/limine-bootloader/limine.git --branch=v3.0-branch-binary --depth=1
	make -C limine

.PHONY: run
run:	
	@echo Running kernel on QEMU
	qemu-system-x86_64 $(QEMU_FLAGS) \
	-bios /usr/share/ovmf/OVMF.fd \
	-cdrom $(OSNAME).iso

.PHONY: qemudebug
qemudebug:
	@echo Launched QEMU with GDB server on localhost:1234"
	@qemu-system-x86_64 -s -S $(QEMU_FLAGS) \
	-bios /usr/share/ovmf/OVMF.fd \
	-cdrom $(OSNAME.iso

.PHONY: debug
debug:
	@echo "Starting remote debugger on localhost:1234"
	@gdb	-ex "set breakpoint pending on" \
			-ex "file kernel/bin/kernel.elf" \
			-ex "b kernelMain" \
			-ex "set disassembly-flavor intel" \
			-ex "target remote :1234" \
			-ex "continue"
