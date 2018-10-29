source ./environment-setup-i586-poky-linux

#qmu-system-i386 -gdb tcp::5501 -S -nographic -kernel bzImage-qemux86.bin -drive file=core-image-lsb-sdk-qemux86.ext4,if=virtio -enable-kvm -net none -usb -localtime --no-reboot --append "root=/dev/vda rw console=ttyS0 evelator=clook"

qemu-system-i386 -gdb tcp::5501 -nographic -kernel /scratch/fall2018/group1/linux-yocto-3.19/arch/x86/boot/bzImage -drive file=core-image-lsb-sdk-qemux86.ext4 -enable-kvm -net none -usb -localtime --no-reboot --append "root=/dev/hda rw console=ttyS0 debug elevator=clook"

