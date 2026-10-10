#!/bin/bash
# Debug shell images for the 6.1 bring-up: ruan/dbgshell/init as the
# ramdisk's init, with busybox and rb.
#
#   mkdbgshell.sh [stage]   -> work/rom61/bootshell-dbg.img, vendor_bootshell<stage>.img
#
# Needs a debug kernel in out-61d (build61.sh with ruan_debug.config) and
# a static aarch64 busybox in tools/busybox-1.36.1.
set -euo pipefail
cd /mnt/directstorage/kernel-src
W=$PWD/work/rom61
D=$PWD/uke61/ruan/dbgshell
STAGE=${1:-}
CMD=${STAGE:+dbg_stage=$STAGE}
O=$PWD/out-61d DEBUG=1 SUFFIX=shell${STAGE} CMDLINE="$CMD" work/mkimg61.sh >/dev/null 2>&1
rm -rf $W/bsh && mkdir $W/bsh && cd $W/bsh
magiskboot unpack ../bootshell${STAGE}.img >/dev/null 2>&1
cp $PWD/../../../tools/busybox-1.36.1/busybox busybox
magiskboot cpio ramdisk.cpio "add 0750 init $D/init" "mkdir 0755 bin" \
	"add 0755 bin/busybox busybox" "add 0755 bin/rb $D/rb" \
	"add 0644 dbgshell.load $D/dbgshell.load" >/dev/null 2>&1
magiskboot repack ../bootshell${STAGE}.img ../bootshell-dbg.img >/dev/null 2>&1
rm -f ../bootshell${STAGE}.img
ls -la $W/bootshell-dbg.img $W/vendor_bootshell${STAGE}.img
