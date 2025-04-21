#!/bin/bash


DRIVE=$1
if [ -z "$DRIVE" ]; then
    echo "Usage: $0 <drive>"
    exit 1
fi
if [ ! -b "$DRIVE" ]; then
    echo "Error: $DRIVE is not a valid block device."
    exit 1
fi
if [ ! -e "$DRIVE" ]; then
    echo "Error: $DRIVE does not exist."
    exit 1
fi

#array of various mkfs commands
mkfs_cmds=(
    "mkfs.xfs $DRIVE -f"
    "mkfs.xfs $DRIVE -f -d agcount=32"
    "mkfs.xfs $DRIVE -f -i sparse=0"
    "mkfs.xfs $DRIVE -f -i sparse=0 -d agcount=32"
    "mkfs.ext4 $DRIVE"
    "mkfs.ext4 -b 8192 $DRIVE -F"
    "mkfs.ext4 -I 256 $DRIVE -F"
    "mkfs.ext4 -b 8192 -I 256 $DRIVE -F"
)

FILESTORM_CMDS=(
    'filestorm aging -d /mnt/test -o true -t 120m --create-dir true'
    'filestorm aging -d /mnt/test -o true -t 20m --rapid-aging true --create-dir true --rapid-aging-threshold 30'
    'filestorm aging -d /mnt/test -o true -t 120m --create-dir true'
    'filestorm aging -d /mnt/test -o true -t 20m --rapid-aging true --create-dir true --rapid-aging-threshold 30'
)

FILESTORM_BLOCKSIZES=(
    "4k"
    "64k"
)

FIO_CMDS=(
    "fio --name=write --ioengine=libaio --rw=write --size=30G --numjobs=1 --runtime=1m --time_based --group_reporting --directory=/mnt/test --direct=1"
)

FIO_BLOCKSIZES=(
    "4k"
    "64k"
)

i=0
for filestorm_cmd in "${FILESTORM_CMDS[@]}"; do
    for cmd in "${mkfs_cmds[@]}"; do
        mkdir -p ~/tests/$i
        cd ~/tests/$i || { echo "Failed to change directory to ~/tests/$i"; exit 1; }

        for blocksize in "${FILESTORM_BLOCKSIZES[@]}"; do
            wipefs -a "$DRIVE" || { echo "Error: wipefs failed."; continue; }
            echo "Running: $cmd"
            $cmd || { echo "Error: $cmd failed."; continue; }
            # Mount the drive
            mount "$DRIVE" /mnt || { echo "Error: mount $DRIVE /mnt failed."; continue; }

            # Run filestorm with blocksize
            echo "Running: $filestorm_cmd --blocksize $blocksize"
            $filestorm_cmd -b $blocksize --save-to "result_${blocksize}.json" || { echo "Error: $filestorm_cmd --blocksize $blocksize failed."; continue; }

            # Unmount the drive
            umount /mnt || { echo "Error: umount /mnt failed."; continue; }

            # Log the command execution
            echo "mkfs: $cmd" >> ~/tests/$i/cmd.txt
            echo "filestorm: $filestorm_cmd -b $blocksize" >> ~/tests/$i/cmd.txt
        done



        #===========================FIO===========================
        # Recreate the directory for fio

        wipefs -a "$DRIVE" || { echo "Error: wipefs failed."; continue; }
        echo "Running: $cmd"
        $cmd || { echo "Error: $cmd failed."; continue; }
        # Mount the drive
        mount "$DRIVE" /mnt || { echo "Error: mount $DRIVE /mnt failed."; continue; }
        mkdir -p /mnt/test
        fio_counter=0
        for fio_cmd in "${FIO_CMDS[@]}"; do
            for fio_blocksize in "${FIO_BLOCKSIZES[@]}"; do
            # Run fio with blocksize
            echo "Running: $fio_cmd --bs=$fio_blocksize"
            $fio_cmd --bs=$fio_blocksize > ~/tests/$i/fio_${fio_blocksize}_$fio_counter.log 2>&1 || { echo "Error: $fio_cmd --bs=$fio_blocksize failed."; continue; }
            # Log the command execution
            echo "fio: $fio_cmd --bs=$fio_blocksize" >> ~/tests/$i/cmd.txt
            fio_counter=$((fio_counter + 1))
            done
        done
        # Unmount the drive
        umount /mnt || { echo "Error: umount /mnt failed."; continue; }


        i=$((i+1))
    done
done