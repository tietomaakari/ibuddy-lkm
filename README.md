Linux device driver for i-Buddy
===============================

http://www.i-buddy.com/

## Before installation

Tested on Ubuntu 11.10 with kernel 3.0.0

add a new file /etc/modprobe.d/usbhid.conf with contents:

	options usbhid quirks=0x1130:0x0001:0x0004

This removes the i-buddy from the control of usbhid subsystem.

If your system uses initrd, that should be updated also:

	update-initramfs -u
	
You'll have to reboot the machine (or uninstall and reload 'usbhid' module)
before usbhid stops claiming the device.

## Compile and install

	make
	insmod ibuddy.ko

## Usage

Plug in i-buddy and check that the directory /proc/driver/ibuddy/0 
is created. If not, check dmesg for error messages.

This directory contains following files: heart, red, green, blue, wings, twist.
If you write _anything_ to these files, they will toggle the 
corresponding "feature" in i-buddy. Written data itself is ignored.

You can test this e.g. in command line:

	echo 1 > /proc/driver/ibuddy/0/heart

You can also read from the attributes - they will always return a hex value
corresponding to the data sent to the i-buddy
(see http://imakethin.gs/blog/?p=17 for explanation of the bits).

Writing to an attribute named reset will return all values to their initial
settings.
