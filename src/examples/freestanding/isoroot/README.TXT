README.TXT for "ASM with Freestanding C" example root directory
Bryan E. Topp <betopp@betopp.com> 2024

This is the filesystem root directory, which is given to mkisofs to make the ISO filesystem image.

It contains the executable for the system to boot, BOOT.NNE. This file should be referenced by the ElTorito boot catalog; command-line switches to mkisofs accomplish this.

Note that the executable could be named anything. It is located on the card using the ElTorito descriptor + catalog, not the filesystem tree.

