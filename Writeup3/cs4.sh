cd linux
make clean
KERNEL=kernel8
make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcmrpi3_defconfig
make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- all