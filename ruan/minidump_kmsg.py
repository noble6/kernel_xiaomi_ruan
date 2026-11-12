#!/usr/bin/env python3
"""Print the kernel log saved in ruan's rawdump minidump (Linux 6.6).

    ruan/minidump_kmsg.py rawdump.bin

Pull rawdump.bin first, as root:
    dd if=/dev/block/by-name/rawdump bs=4M

The 6.6 kernel registers its printk ring buffer in the minidump as three
regions (KLOGBUF, LOG_DESC, LOG_INFO, see qcom_logbuf_vendor_hooks.c);
this decodes them back into a log ordered by sequence number.
"""
import struct
import sys


def sections(d):
    if d[:8] != b'Raw_Dmp!':
        sys.exit('not a minidump')
    out, off = {}, 0x58
    while off + 0x40 <= len(d):
        valid, _, _, start, size, _ = struct.unpack_from('<IIIQQQ', d, off)
        name = d[off + 0x2c:off + 0x40].split(b'\0')[0].decode('latin1')
        if valid != 1 or not name:
            break
        out[name] = d[start:start + size]
        off += 0x40
    return out


def main():
    s = sections(open(sys.argv[1], 'rb').read())
    try:
        text, desc, info = (s[n] for n in ('md_KLOGBUF.BIN', 'md_LOG_DESC.BIN',
                                           'md_LOG_INFO.BIN'))
    except KeyError:
        sys.exit('no kernel log in this dump (sections: %d)' % len(s))
    n = len(desc) // 24
    isz, tsz = len(info) // n, len(text)
    recs = []
    for i in range(n):
        _, begin, nxt = struct.unpack_from('<QQQ', desc, i * 24)
        seq, ts, tlen = struct.unpack_from('<QQH', info, i * isz)
        if not tlen or begin == nxt:
            continue
        b, e = begin % tsz, nxt % tsz
        if b >= e and e:
            b = 0
        recs.append((seq, ts, text[b + 8:b + 8 + tlen].decode('utf-8', 'replace')))
    for _, ts, msg in sorted(recs):
        for line in msg.split('\n'):
            print(f'[{ts / 1e9:12.6f}] {line}')


if __name__ == '__main__':
    main()
