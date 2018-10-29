cd /scratch/fall2018/group1/linux-yocto-3.19/

make clean

cp /scratch/files/config-3.19.2-yocto-standard .config
# press y to overwrite

make -j4 all > log_res.txt
