#!/bin/bash
set -e

echo "== Ext2 Test =="

sudo umount -f mnt 2>/dev/null || true
rm -rf mnt ext2.img file*

echo "Make 1000MB image"
truncate -s 1000M ext2.img

echo "Format, mount"
mkfs.ext2 ext2.img
mkdir -p ./mnt
sudo mount ext2.img mnt
sudo chown "$(id -u):$(id -g)" mnt

echo "Create test files"
echo "Hello Ext2!" > mnt/file1.txt
dd if=/dev/urandom of=mnt/file2.bin bs=1M count=2 status=none
dd if=/dev/urandom of=mnt/file3.bin bs=1M count=300 status=none

FILE1_INODE=$(ls -i mnt/file1.txt | awk '{print $1}')
FILE2_INODE=$(ls -i mnt/file2.bin | awk '{print $1}')
FILE3_INODE=$(ls -i mnt/file3.bin | awk '{print $1}')

FILE1_HASH=$(sha512sum mnt/file1.txt | awk '{print $1}')
FILE2_HASH=$(sha512sum mnt/file2.bin | awk '{print $1}')
FILE3_HASH=$(sha512sum mnt/file3.bin | awk '{print $1}')

echo "Unmount image"
sudo umount mnt

./main ext2.img "$FILE1_INODE" > file1_extracted.txt
./main ext2.img "$FILE2_INODE" > file2_extracted.bin
./main ext2.img "$FILE3_INODE" > file3_extracted.bin

LOOP_DEV=$(sudo losetup -f --show ext2.img)
sudo ./main "$LOOP_DEV" "$FILE1_INODE" > file1_loop.txt
sudo losetup -d "$LOOP_DEV"

echo "Tests:"

echo "File1"
if [ "$FILE1_HASH" = "$(sha512sum file1_extracted.txt | awk '{print $1}')" ]; then
    echo "PASSED"
else
    echo "FAILED"
fi

echo "File2"
if [ "$FILE2_HASH" = "$(sha512sum file2_extracted.bin | awk '{print $1}')" ]; then
    echo "PASSED"
else
    echo "FAILED"
fi

echo "File3"
if [ "$FILE3_HASH" = "$(sha512sum file3_extracted.bin | awk '{print $1}')" ]; then
    echo "PASSED"
else
    echo "FAILED"
fi

echo "File1 (loop)"
if [ "$FILE1_HASH" = "$(sha512sum file1_loop.txt | awk '{print $1}')" ]; then
    echo "PASSED"
else
    echo "FAILED"
fi

echo "Cleanup"
rm -rf mnt ext2.img file*
