dd if=/dev/zero of=fat.fs bs=1024 count=1024
mkfs.vfat fat.fs
mkdir -p mnt
sudo mount -o loop  fat.fs ./mnt
cd image
sudo cp -R * ../mnt
ls -alR
cd ..
sudo umount ./mnt

