#!/bin/bash
# vbprobe.sh N...: vendor_boot images that power off at checkpoint N, from
# the last `SUFFIX=probe mkimg61.sh` run (same ramdisk and dtb).
set -euo pipefail
W=/mnt/directstorage/kernel-src/work/rom61
for n in "$@"; do
python3 -I - "$W" "$n" <<'PY'
import shlex, subprocess, sys
w, n = sys.argv[1:]
args = shlex.split(subprocess.run(['unpack_bootimg', '--boot_img', w + '/vendor_bootprobe.img',
                                   '--out', w + '/vbp', '--format', 'mkbootimg'],
                                  capture_output=True, text=True, check=True).stdout)
out = []
i = 0
while i < len(args):
    if args[i] == '--vendor_cmdline':
        c = ' '.join(t for t in args[i + 1].split() if not t.startswith('ruan_probe='))
        out += [args[i], c + ' ruan_probe=' + n]; i += 2; continue
    out.append(args[i]); i += 1
subprocess.run(['mkbootimg'] + out + ['--vendor_boot', f'{w}/vendor_bootprobe{n}.img'], check=True)
PY
echo "$W/vendor_bootprobe$n.img"
done
