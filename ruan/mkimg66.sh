#!/bin/bash
# Build test boot and vendor_boot images for the 6.6 kernel.
#
#   ruan/mkimg66.sh <dir with the ROM's boot.img and vendor_boot.img>
#
# Takes the installed ROM's images and swaps in the 6.6 kernel and dtb.
# The kernel is monolithic, so the 5.10 modules are removed from the
# vendor ramdisk and first stage init loads nothing. The ROM's 5.10
# modules on vendor_dlkm then fail to load later, which is harmless.
#
# Environment:
#   O        kernel build dir, default ../out-66
#   SUFFIX   appended to the output names, default 66
#   CMDLINE  appended to the vendor command line
#
# Output: boot$SUFFIX.img and vendor_boot$SUFFIX.img next to the inputs.
set -euo pipefail
K=$(cd "$(dirname "$0")/.." && pwd)
W=$(cd "${1:?ROM image dir}" && pwd)
O=${O:-$K/../out-66}
SUFFIX=${SUFFIX:-66}
T=$W/tmp$SUFFIX
rm -rf "$T" && mkdir -p "$T"

python3 -I - "$W" "$T" "$SUFFIX" "$O/arch/arm64/boot/dts/ruan/ruan.dtb" "${CMDLINE:-}" <<'EOF'
import os, shlex, subprocess, sys
w, t, suffix, dtb, extra = sys.argv[1:]

def run(*a, **kw):
    return subprocess.run(a, check=True, **kw)

args = shlex.split(run('unpack_bootimg', '--boot_img', w + '/vendor_boot.img',
                       '--out', t + '/vb', '--format', 'mkbootimg',
                       capture_output=True, text=True).stdout)

def strip_modules(src, dst):
    """Unpack a vendor ramdisk fragment, drop its modules, pack it again."""
    d = dst + '.d'
    os.makedirs(d)
    raw = open(src, 'rb').read()
    if raw[:4] in (b'\x02\x21\x4c\x18', b'\x04\x22\x4d\x18'):
        raw = run('lz4', '-dc', src, capture_output=True).stdout
    elif raw[:2] == b'\x1f\x8b':
        raw = run('gzip', '-dc', src, capture_output=True).stdout
    run('cpio', '-idm', '--quiet', input=raw, cwd=d)
    n = 0
    for root, _, files in os.walk(d):
        if os.path.basename(root) != 'modules':
            continue
        for f in files:
            p = os.path.join(root, f)
            if f.endswith('.ko'):
                os.remove(p); n += 1
            elif f.startswith('modules.'):
                open(p, 'w').close()
    print(f'{os.path.basename(src)}: removed {n} modules')
    files = run('find', '.', capture_output=True, text=True, cwd=d).stdout
    files = ''.join(sorted(l + '\n' for l in files.split('\n') if l))
    cpio = run('cpio', '-o', '-H', 'newc', '--quiet', '-R', '0:0',
               input=files.encode(), capture_output=True, cwd=d).stdout
    open(dst, 'wb').write(run('lz4', '-l', '-12', '--favor-decSpeed', '-q', '-c',
                              input=cpio, capture_output=True).stdout)

out, i, frag = [], 0, 0
while i < len(args):
    a = args[i]
    if a == '--dtb':
        out += [a, dtb]; i += 2; continue
    if a == '--vendor_ramdisk_fragment':
        dst = f'{t}/vendor_ramdisk{frag:02d}'
        strip_modules(args[i + 1], dst)
        out += [a, dst]; frag += 1; i += 2; continue
    if a == '--vendor_cmdline' and extra:
        out += [a, args[i + 1] + ' ' + extra]; i += 2; continue
    out.append(a); i += 1
run('mkbootimg', *out, '--vendor_boot', f'{w}/vendor_boot{suffix}.img')
EOF

# boot: the ROM's generic ramdisk with the 6.6 kernel
mkdir -p "$T"/b
(cd "$T"/b && magiskboot unpack "$W"/boot.img >/dev/null 2>&1 \
	&& cp "$O"/arch/arm64/boot/Image kernel \
	&& magiskboot repack "$W"/boot.img "$W"/boot$SUFFIX.img >/dev/null 2>&1)

ls -la "$W"/boot$SUFFIX.img "$W"/vendor_boot$SUFFIX.img
echo "kernel $(cat "$O"/include/config/kernel.release)"
