KERNEL SANDERS

This is my very small RISC-V kernel-esqe program!
It's designed to run on qemu, and right now just displays an image
of the Colonel himself, hence the name.

For the future, I plan to expand this kernel to function like an
actual kernel, with user processes and some form of IO (most likely
based on xv6). And maybe it will display a video instead of an
image. But for now, this rudimentary bare-metal program displays 
an image of Colonel Sanders.

TO RUN:
make qemu

make sure you have the required RISC-V build tools installed as well
