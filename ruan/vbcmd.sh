#!/bin/bash
# vbcmd.sh <base vendor_boot> <out> <extra cmdline>: same vendor_boot, more cmdline
set -euo pipefail
python3 -I - "$@" <<'PY'
import shlex, subprocess, sys
base, out_img, extra = sys.argv[1:]
args = shlex.split(subprocess.run(['unpack_bootimg', '--boot_img', base, '--out', out_img + '.d',
                                   '--format', 'mkbootimg'], capture_output=True, text=True, check=True).stdout)
out = []
i = 0
while i < len(args):
    if args[i] == '--vendor_cmdline':
        out += [args[i], args[i + 1] + ' ' + extra]; i += 2; continue
    out.append(args[i]); i += 1
subprocess.run(['mkbootimg'] + out + ['--vendor_boot', out_img], check=True)
PY
