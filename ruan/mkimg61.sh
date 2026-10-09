#!/bin/bash
# Build test boot.img + vendor_boot.img for the experimental 6.1 kernel.
#
# Starts from the ROM's boot.img and vendor_boot.img (work/rom61) and
# replaces the kernel, the vendor ramdisk modules and the dtb. All 6.1
# modules go into the vendor ramdisk and first stage init loads them all;
# the 5.10 modules on vendor_dlkm then fail to load, which is harmless.
set -euo pipefail
cd /mnt/directstorage/kernel-src
W=$PWD/work/rom61
K=$PWD/uke61
O=$PWD/out-61
export PATH=$PWD/clang-r487747c/bin:$PATH
REL=$(cat $O/include/config/kernel.release)

# modules: install, flatten, depmod
S=$W/stage
rm -rf $S
make -s -C $K O=$O ARCH=arm64 LLVM=1 LLVM_IAS=1 CROSS_COMPILE=aarch64-linux-gnu- \
	INSTALL_MOD_PATH=$S INSTALL_MOD_STRIP=1 modules_install 2>/dev/null
F=$W/flat/lib/modules/0.0
rm -rf $W/flat && mkdir -p $F
find $S -name '*.ko' -exec cp {} $F/ \;
depmod -b $W/flat 0.0

# vendor ramdisk: stock contents with the 6.1 modules
R=$W/vr61
rm -rf $R && cp -a $W/vr $R
rm -f $R/lib/modules/*
cp $F/*.ko $R/lib/modules/
sed -E 's#([^ :]+\.ko)#/lib/modules/\1#g' $F/modules.dep > $R/lib/modules/modules.dep
cp $F/modules.alias $F/modules.softdep $R/lib/modules/
# load order: stock's first stage order (with 6.1 names), then the rest
python3 -I - "$W/vr/lib/modules/modules.load" "$R/lib/modules" <<'EOF'
import os, sys
stock, mdir = sys.argv[1:]
renamed = {'crypto-qti-common.ko': 'crypto-qti.ko', 'crypto-qti-hwkm.ko': 'hwkm.ko',
           'qcom_ipcc.ko': 'qcom-ipcc.ko'}
have = sorted(f for f in os.listdir(mdir) if f.endswith('.ko'))
order = []
for l in open(stock):
    m = renamed.get(l.strip(), l.strip())
    if m in have and m not in order:
        order.append(m)
order += [m for m in have if m not in order]
for name in ('modules.load', 'modules.load.recovery'):
    open(os.path.join(mdir, name), 'w').write('\n'.join(order) + '\n')
open(os.path.join(mdir, 'modules.blocklist'), 'w').write('')
print(len(order), 'modules in modules.load')
EOF
(cd $R && find . | sort | cpio -o -H newc --quiet -R 0:0) | lz4 -l -12 --favor-decSpeed -q > $W/vendor_ramdisk61

# dtb
dtc -q -I dts -O dtb -o $W/ruan.dtb $K/arch/arm64/boot/dts/ruan/ruan.dts

# vendor_boot: stock header values, our ramdisk and dtb
python3 -I - "$W" <<'EOF'
import shlex, subprocess, sys
w = sys.argv[1]
args = shlex.split(subprocess.run(['unpack_bootimg', '--boot_img', w + '/vendor_boot.img',
                                   '--out', w + '/vb', '--format', 'mkbootimg'],
                                  capture_output=True, text=True, check=True).stdout)
out = []
i = 0
while i < len(args):
    a = args[i]
    if a == '--dtb':
        out += [a, w + '/ruan.dtb']; i += 2; continue
    if a == '--vendor_ramdisk_fragment':
        out += [a, w + '/vendor_ramdisk61']; i += 2; continue
    out.append(a); i += 1
subprocess.run(['mkbootimg'] + out + ['--vendor_boot', w + '/vendor_boot61.img'], check=True)
EOF

# boot: the ROM's generic ramdisk with the 6.1 kernel
rm -rf $W/b61 && mkdir $W/b61
(cd $W/b61 && magiskboot unpack ../boot.img >/dev/null 2>&1 && cp $O/arch/arm64/boot/Image kernel \
	&& magiskboot repack ../boot.img ../boot61.img >/dev/null 2>&1)

ls -la $W/boot61.img $W/vendor_boot61.img
echo "kernel $REL"
