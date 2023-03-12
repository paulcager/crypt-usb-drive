# A Fairly Safe LUKS Encryption Unlocker

A way to automatically mount an encrypted [LUKS](https://en.wikipedia.org/wiki/Linux_Unified_Key_Setup) filesystem, while still preserving
some level of security.

## What Is It?

This firmware runs on a [Pico-W](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf) and emulates
a FAT partition containing a
LUKS [keyfile](https://man7.org/linux/man-pages/man5/crypttab.5.html#EXAMPLES). The keyfile can be used to unlock
the LUKS partition without having to enter a password. This isn't something you would normally want to do,
as, in general, it defeats the purpose of encrypting the disks.

## How to build it.

* Run `make-disk-image` to generate a disk image in files `image.c` and `image.h`.
* Create a 64 byte random file on an HTTP server (preferably not exposed to the public web):
** `dd if=/dev/random bs=64 count=1 of=my-key-file.bin`
* Copy `secrets-example.h` to `secrets.h` and edit it to match your environment.
* Build and flash to a pico_w in the normal way.

## How to use it.

Add the Pico's file as a LUKS key (the UUID is for a LUKS filesystem):
```shell
mkdir /tmp/mnt
mount -o ro /dev/disk/by-label/KEYDISK /tmp/mnt
cryptsetup luksAddKey UUID=XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX /tmp/mnt/key
umount /tmp/mnt
```

> Always have a second way of unlocking the filesystem, such as a passphrase.

Edit `/etc/crypttab` to add the key automatically on reboot:
```shell
data    UUID=XXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX       key:LABEL=KEYDISK       luks
```

## What sort of security do you get?

The level of security is good enough for my use. To use it to unlock the filesystem you'd need:

* The pico itself, or else knowledge of the contents of `FILE_PREFIX` (and Pico serial number / MAC address).
* Access to `KEY_SERVER`.