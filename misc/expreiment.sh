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
    'filestorm aging -b 4k -d /mnt/test -o true -t 120m --create-dir true'
    'filestorm aging -b 4k -d /mnt/test -o true -t 20m --rapid-aging true --create-dir true --rapid-aging-threshold 30'
)
FIO_CMDS=(
    "fio --name=write --ioengine=libaio --rw=write --bs=64k --size=30G --numjobs=1 --runtime=1m --time_based --group_reporting --directory=/mnt/test --direct=1"
    "fio --name=write --ioengine=libaio --rw=write --bs=4k --size=30G --numjobs=1 --runtime=1m --time_based --group_reporting --directory=/mnt/test --direct=1"
)
i=0
for filestorm_cmd in "${FILESTORM_CMDS[@]}"; do
    for cmd in "${mkfs_cmds[@]}"; do
        mkdir -p ~/tests/$i
        cd ~/tests/$i || { echo "Failed to change directory to ~/tests/$i"; exit 1; }

        wipefs -a "$DRIVE" || { echo "Error: wipefs failed."; continue; }
        echo "Running: $cmd"
        $cmd || { echo "Error: $cmd failed."; continue; }
        # Mount the drive
        mount "$DRIVE" /mnt || { echo "Error: mount $DRIVE /mnt failed."; continue; }

        # Run filestorm
        echo "Running: $filestorm_cmd"
        $filestorm_cmd || { echo "Error: $filestorm_cmd failed."; continue; }

        # Unmount the drive
        umount /mnt || { echo "Error: umount /mnt failed."; continue; }

        # Log the command execution
        echo "mkfs: $cmd" >> ~/tests/$i/cmd.txt
        echo "filestorm: $filestorm_cmd" >> ~/tests/$i/cmd.txt



        #===========================FIO===========================
        # Recreate the directory for fio

        wipefs -a "$DRIVE" || { echo "Error: wipefs failed."; continue; }
        echo "Running: $cmd"
        $cmd || { echo "Error: $cmd failed."; continue; }
        # Mount the drive
        mount "$DRIVE" /mnt || { echo "Error: mount $DRIVE /mnt failed."; continue; }

        fio_counter=0
        for fio_cmd in "${FIO_CMDS[@]}"; do
            # Run fio
            echo "Running: $fio_cmd"
            $fio_cmd > ~/tests/$i/fio_$fio_counter.log 2>&1 || { echo "Error: $fio_cmd failed."; continue; }
            # Log the command execution
            echo "fio: $fio_cmd" >> ~/tests/$i/cmd.txt
            fio_counter=$((fio_counter + 1))
        done
        # Unmount the drive
        umount /mnt || { echo "Error: umount /mnt failed."; continue; }


        i=$((i+1))
    done
done