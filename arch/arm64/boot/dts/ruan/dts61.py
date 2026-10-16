#!/usr/bin/env python3
"""Convert ruan's stock 5.10 device tree to the bindings of the 6.1 drivers.

    dts61.py ruan-stock-5.10.dts ruan.dts

The input is dtc's decompiled output: one property or node per line,
numeric phandles. Each conversion is a function below; they run in order.
"""
import re
import sys


class Node:
    def __init__(self, name, labels=()):
        self.name = name
        self.labels = list(labels)
        self.props = []      # [name, value or None]
        self.children = []
        self.parent = None

    def prop(self, name):
        for k, v in self.props:
            if k == name:
                return v
        return None

    def set(self, name, value):
        for p in self.props:
            if p[0] == name:
                p[1] = value
                return
        self.props.append([name, value])

    def delete(self, name):
        self.props = [p for p in self.props if p[0] != name]

    def add(self, child):
        child.parent = self
        self.children.append(child)

    def remove(self, child):
        self.children.remove(child)
        child.parent = None

    def walk(self):
        yield self
        for c in self.children:
            yield from c.walk()

    def path(self):
        if self.parent is None:
            return ''
        return self.parent.path() + '/' + self.name

    def compatible(self):
        v = self.prop('compatible')
        return re.findall(r'"([^"]*)"', v) if v else []


NODE_RE = re.compile(r'^((?:[\w-]+:\s*)*)([\w@,.+/-]+)\s*\{$')
PROP_RE = re.compile(r'^([#\w,.+?-]+)(?:\s*=\s*(.*))?;$')


def parse(text):
    lines = [l.strip() for l in text.split('\n')]
    header = []
    root = None
    stack = []
    for l in lines:
        if not l:
            continue
        if root is None and (l.startswith('/dts-v1/') or l.startswith('/memreserve/')):
            header.append(l)
            continue
        m = NODE_RE.match(l)
        if m:
            labels = re.findall(r'([\w-]+):', m.group(1))
            n = Node(m.group(2), labels)
            if stack:
                stack[-1].add(n)
            else:
                root = n
            stack.append(n)
            continue
        if l == '};':
            stack.pop()
            continue
        m = PROP_RE.match(l)
        if m and stack:
            stack[-1].props.append([m.group(1), m.group(2)])
            continue
        raise ValueError('cannot parse: ' + l)
    return header, root


def emit(header, root):
    out = header[:] + ['']

    def rec(n, depth):
        ind = '\t' * depth
        lab = ''.join(l + ': ' for l in n.labels)
        out.append(f'{ind}{lab}{n.name} {{')
        for k, v in n.props:
            out.append(f'{ind}\t{k};' if v is None else f'{ind}\t{k} = {v};')
        for c in n.children:
            out.append('')
            rec(c, depth + 1)
        out.append(f'{ind}}};')
    rec(root, 0)
    return '\n'.join(out) + '\n'


def cells(v):
    """'<0x1 0x2> , <0x3>' -> ['0x1', '0x2', '0x3']"""
    return re.findall(r'0x[0-9a-fA-F]+|\d+', ''.join(re.findall(r'<([^>]*)>', v or '')))


def by_phandle(root):
    m = {}
    for n in root.walk():
        p = n.prop('phandle')
        if p:
            m[int(cells(p)[0], 0)] = n
    return m


def strings(v):
    return re.findall(r'"([^"]*)"', v or '')


# --------------------------------------------------------------------------
# Conversions

MOVED = {}  # old node path -> new node path


def qup_wrappers(root):
    """QUPv3: serial engines become children of their wrapper.

    5.10: wrapper "qcom,qupv3-geni-se" and standalone serial engines that
    point to it with qcom,wrapper-core, each with se-clk, m-ahb and s-ahb.
    6.1: wrapper "qcom,geni-se-qup" holding the m-ahb and s-ahb clocks,
    serial engines as its children (they use dev->parent).
    """
    ph = by_phandle(root)
    moved = 0
    for se in list(root.walk()):
        wc = se.prop('qcom,wrapper-core')
        if not wc:
            continue
        wrapper = ph[int(cells(wc)[0], 0)]
        if wrapper.prop('compatible') == '"qcom,qupv3-geni-se"':
            wrapper.set('compatible', '"qcom,geni-se-qup"')
            wrapper.set('#address-cells', '<0x01>')
            wrapper.set('#size-cells', '<0x01>')
            wrapper.set('ranges', None)
        if wrapper.prop('clocks') is None:
            names = strings(se.prop('clock-names'))
            clk = cells(se.prop('clocks'))
            per = len(clk) // len(names)
            ahb = []
            for want in ('m-ahb', 's-ahb'):
                i = names.index(want)
                ahb += clk[i * per:(i + 1) * per]
            wrapper.set('clock-names', '"m-ahb", "s-ahb"')
            wrapper.set('clocks', '<' + ' '.join(ahb) + '>')
        se.delete('qcom,wrapper-core')
        old = se.path()
        se.parent.remove(se)
        wrapper.add(se)
        MOVED[old] = se.path()
        moved += 1
    return f'{moved} serial engines moved under their wrappers'


def rpmh_rsc(root):
    """RPMh RSCs: one child node per DRV, the clients under it.

    5.10: a flat "qcom,rpmh-rsc" node with qcom,drv-id, qcom,tcs-offset and
    qcom,tcs-config, its clients (RPMh clocks, BCM voters, regulators) as
    children.
    6.1: qcom,drv-count on the RSC, a "drv" child per DRV with qcom,drv-id
    and qcom,tcs-offset, a "channel" child of that with qcom,tcs-config,
    and the clients under the drv node: they find their controller through
    their parent. reg and interrupts stay indexed by DRV id.
    """
    n = 0
    for rsc in list(root.walk()):
        if 'qcom,rpmh-rsc' not in rsc.compatible() or rsc.prop('qcom,drv-count'):
            continue
        drv = Node('drv')
        drv.props = [['qcom,drv-id', rsc.prop('qcom,drv-id')],
                     ['qcom,tcs-offset', rsc.prop('qcom,tcs-offset')]]
        channel = Node('channel')
        channel.props = [['qcom,tcs-config', rsc.prop('qcom,tcs-config')]]
        drv.add(channel)
        # Neither the 5.10 nor the 6.1 driver uses the RSC's clocks; as a
        # fw_devlink supplier (dispcc for disp_rsc) they only held the RSC,
        # its BCM voter and every NoC voting through it back until the
        # deferred probe timeout, after first stage init gave up on UFS.
        for name in ('qcom,drv-id', 'qcom,tcs-offset', 'qcom,tcs-config', 'clocks'):
            rsc.delete(name)
        rsc.set('qcom,drv-count', f'<{len(strings(rsc.prop("reg-names")))}>')
        clients = list(rsc.children)
        rsc.add(drv)
        for client in clients:
            old = client.path()
            rsc.remove(client)
            drv.add(client)
            MOVED[old] = client.path()
        n += 1
    return f'{n} RSCs converted'


def icc_skip_qos(root):
    """Bring-up: don't program NoC QoS.

    Once RPMh works the parrot interconnect providers register and the
    6.1 driver writes QoS registers on every NoC; the boot hangs right
    there (no panic, watchdog reset). QoS is only priority tuning, so
    skip it until the NoC clocks are sorted out.
    """
    n = 0
    for node in root.walk():
        if node.prop('#interconnect-cells') is not None and \
                any(c.startswith('qcom,parrot-') for c in node.compatible()) and \
                node.prop('qcom,skip-qos') is None:
            node.set('qcom,skip-qos', None)
            n += 1
    return f'{n} NoCs'


def smmu_tbus(root):
    """QSMMUv500 TBUs: 6.1 requires qcom,iova-width.

    5.10 defaulted to 36 bits (QCOM_IOVA_WIDTH_DEFAULT) when it was
    missing, as in ruan's DT; 6.1 fails the TBU probe with -EINVAL and
    the SMMU gives up ("TBU failed probe, QSMMUV500 cannot continue!"),
    leaving UFS and every other IOMMU user deferred.
    """
    n = 0
    for node in root.walk():
        if 'qcom,qsmmuv500-tbu' in node.compatible() and node.prop('qcom,iova-width') is None:
            node.set('qcom,iova-width', '<36>')
            n += 1
    return f'{n} TBUs'


def node_paths(root):
    """Point __symbols__, aliases and chosen at the nodes' new paths.

    The bootloader applies the ruan overlay from dtbo.img on top of this
    dtb, resolving its labels through __symbols__.
    """
    n = 0
    for name in ('__symbols__', 'aliases', 'chosen'):
        node = next((c for c in root.children if c.name == name), None)
        if node is None:
            continue
        for p in node.props:
            if not p[1] or not p[1].startswith('"/'):
                continue
            path = p[1].strip('"')
            for old, new in MOVED.items():
                if path == old or path.startswith(old + '/'):
                    p[1] = '"' + new + path[len(old):] + '"'
                    n += 1
                    break
    return f'{n} paths updated'


RENAMED = {
    # EPSS cpufreq: same registers, 6.1 name
    'qcom,cpufreq-hw-epss': 'qcom,cpufreq-epss',
}


def compatibles(root):
    """Compatibles whose hardware has another name on 6.1."""
    n = 0
    for node in root.walk():
        v = node.prop('compatible')
        if not v:
            continue
        new = v
        for old, to in RENAMED.items():
            new = new.replace(f'"{old}"', f'"{to}"')
        if new != v:
            node.set('compatible', new)
            n += 1
    return f'{n} renamed'


def soc_ids(root):
    """Let the bootloader pick this dtb on both Parrot and ParrotP.

    ruan ships with both "Parrot SoC" (msm-id 0x219, 0x265) and "ParrotP
    SoC" (0x277, 0x247, 0x27e) dtbs, which differ only in the root
    compatible.
    """
    root.set('qcom,msm-id', '<0x219 0x10000 0x265 0x10000 0x277 0x10000 '
             '0x247 0x10000 0x27e 0x10000>')
    root.set('compatible', '"qcom,parrot"')
    return 'Parrot and ParrotP'


def splash_console(root):
    """Debug builds (DEBUG_FB=1): draw the kernel console on the splash.

    The bootloader leaves the video mode panel scanning out its splash
    buffer; describing it as a simple-framebuffer gives fbcon a screen
    before any display driver loads.
    """
    import os
    if os.environ.get('DEBUG_FB') != '1':
        return 'off'
    chosen = next(c for c in root.children if c.name == 'chosen')
    chosen.set('#address-cells', '<0x02>')
    chosen.set('#size-cells', '<0x02>')
    chosen.set('ranges', None)
    fb = Node('framebuffer@b8000000')
    fb.props = [
        ['compatible', '"simple-framebuffer"'],
        ['reg', '<0x00 0xb8000000 0x00 0x2b00000>'],
        ['width', '<1600>'],
        ['height', '<2560>'],
        ['stride', '<6400>'],
        ['format', '"a8r8g8b8"'],
    ]
    chosen.add(fb)
    return 'simple-framebuffer at the splash region'


CONVERSIONS = [
    qup_wrappers,
    rpmh_rsc,
    icc_skip_qos,
    smmu_tbus,
    compatibles,
    node_paths,
    soc_ids,
    splash_console,
]


def main():
    header, root = parse(open(sys.argv[1]).read())
    for conv in CONVERSIONS:
        print(f'{conv.__name__}: {conv(root)}')
    open(sys.argv[2], 'w').write(emit(header, root))


if __name__ == '__main__':
    main()
