# Linux kernel device driver assignment (MC504 - lab06)

This repository contains only the files that were added or modified on top of a stock Linux kernel source tree (developed and tested against Linux 7.1.0), but it is not a full kernel tree by itself.

Both the module lifecycle work (build system wiring, character device registration) and the overall approach to writing a pseudo device driver in this repository are based on the LKCamp device drivers tutorial: <https://docs.lkcamp.dev/intro_tutorials/device_drivers/>. The `hello` module follows that tutorial directly; the `cipher` module extends its character device pattern with the encryption and `ioctl` configuration mechanism described below, which go beyond what the tutorial itself covers.

## What this is

Two loadable kernel modules live under `drivers/lkcamp/`:

- **`hello`** (`hello.c`): a minimal module following the LKCamp device
  driver tutorial. It registers a character device (`/proc/devices` entry
  named `lkcamp`) whose `read`/`write` hold a simple ON/OFF status string.
  Its purpose is mainly educational, showing the basic module lifecycle
  (`module_init`/`module_exit`) and character device registration
  (`alloc_chrdev_region`, `cdev_init`, `cdev_add`).

- **`cipher`** (`crypto.c`): the actual assignment deliverable, a pseudo
  device driver exposed as `/dev/cipher`. It implements a small in-kernel
  buffer that stores strings *encrypted*, not in plaintext:

  - `write()` copies the string from user space (`copy_from_user`) and
    applies a Caesar-style shift to every byte before storing it.
  - `read()` copies the stored bytes into a local buffer, reverses the
    shift, and hands the decrypted result back to user space
    (`copy_to_user`).
  - The shift key is configurable at runtime through two `ioctl` commands,
    `LKCAMP_SET_KEY` and `LKCAMP_GET_KEY` (defined in
    `lkcamp_ioctl.h`). If a program sets the wrong key before reading data
    that was written with a different key, `read()` returns garbage instead
    of the original string. This is the intended behavior, since it
    demonstrates that the driver's output genuinely depends on its runtime
    configuration, not just on what was written.

## Repository layout

```
drivers/lkcamp/
    Kconfig           Kconfig entries for the HELLO and CIPHER modules
    Makefile          obj-$(CONFIG_...) build rules for hello.o and crypto.o
    hello.c           tutorial module (ON/OFF char device)
    crypto.c          cipher buffer driver (the assignment deliverable)
    lkcamp_ioctl.h    ioctl command definitions shared by crypto.c and the
                      user-space test program

shared_folder/
    test_crypto.c     user-space program that exercises /dev/cipher
    lkcamp_ioctl.h    copy of the ioctl header, so test_crypto.c can be
                      built on its own without the kernel tree
```

`drivers/lkcamp/` is meant to be copied as-is into `<kernel-source>/drivers/lkcamp/`.
It is not wired into the top-level build by itself; two small edits are
needed in files that belong to the kernel tree, not to this repository:

- `drivers/Kconfig`: add `source "drivers/lkcamp/Kconfig"` before the final
  `endmenu` of the file.
- `drivers/Makefile`: add `obj-y += lkcamp/`.

## Building

From the root of a Linux kernel source tree that already has a working
`.config` and has been built at least once:

1. Copy this repository's `drivers/lkcamp/` directory into the kernel tree,
   and make the two `drivers/Kconfig` / `drivers/Makefile` edits described
   above.
2. Enable the modules. Either run `make menuconfig` and set "Hello module"
   and "Cipher module" to `<M>` under Device Drivers, or simply run
   `make olddefconfig`, since both options default to `m` in their Kconfig
   entries.
3. Build just this driver directory:
   ```
   make M=drivers/lkcamp
   ```
   This produces `drivers/lkcamp/hello.ko` and `drivers/lkcamp/crypto.ko`.

## Running the QEMU test VM

The kernel built above was tested by booting it directly under QEMU, from
the root of the kernel source tree, with a raw disk image as the root
filesystem and the shared folder exposed over virtio-9p:

```
qemu-system-x86_64 \
    -drive file=../my_disk.raw,format=raw,index=0,media=disk \
    -m 2G -nographic \
    -kernel ./arch/x86_64/boot/bzImage \
    -append "root=/dev/sda rw console=ttyS0 loglevel=6" \
    -fsdev local,id=fs1,path=../shared_folder,security_model=none \
    -device virtio-9p-pci,fsdev=fs1,mount_tag=shared_folder \
    --enable-kvm
```

This assumes `my_disk.raw` (a disk image with a bootable root filesystem)
and `shared_folder` (the host directory holding `crypto.ko` and
`test_crypto.c`, mounted with tag `shared_folder`) both sit one directory
above the kernel source tree. Inside the guest, the shared folder shows up
as `host_folder`, which is the path used in the loading and testing steps
below.

## Loading and testing

These modules were tested inside a VM booting the kernel built above, with
the host's copy of this repository mounted as a shared folder.

1. Copy `crypto.ko` into the shared folder and, inside the VM, load it:
   ```
   insmod crypto.ko
   ```
2. Find the major number the kernel assigned to the `cipher` device:
   ```
   cat /proc/devices | grep cipher
   ```
3. Create the device node (substitute the major number found above):
   ```
   mknod /dev/cipher c <major> 0
   ```
4. Build and run the test program (it needs `lkcamp_ioctl.h` from
   `shared_folder/` in the same directory, since it does not depend on the
   kernel source tree):
   ```
   gcc test_crypto.c -o test_crypto
   ./test_crypto
   ```

`test_crypto` sets a key, writes a string, reads it back (matching the
original), then sets a different key and reads the same stored data again,
this time printing garbled output. Example run:

```
key set to 5
wrote: LAB06 DE SO
read with correct key: LAB06 DE SO
key set to 9
read with wrong key: H=>,2@AOK
```

The `hello` module can be tested the same way (`insmod hello.ko`, find its
major number under the name `lkcamp` in `/proc/devices`, `mknod`), by
writing `0` or `1` to its device node and reading the resulting `OFF`/`ON`
string back with `cat`.
