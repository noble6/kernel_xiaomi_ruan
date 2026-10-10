# Linux 6.1 for ruan (experimental)

**Not bootable yet.** The release kernel for the Redmi Pad Pro 5G / POCO
Pad 5G (ruan) is 5.10 on the `axion` branch.

This branch ports ruan (Snapdragon 7s Gen 2, parrot) to Linux 6.1 GKI
(android14-6.1). Qualcomm never released parrot on a kernel newer than
5.10, so the base is Xiaomi's 6.1.68 release for the Xiaomi Pad 7
(`uke-v-oss`, Qualcomm's full 6.1 msm kernel), and the parrot SoC
drivers are forward-ported from ruan's 5.10 source (`ruan-u-oss`).

## Building

Needs AOSP clang-r487747c (the android14-6.1 compiler).

```
export PATH=/path/to/clang-r487747c/bin:$PATH
M="make O=out ARCH=arm64 LLVM=1 LLVM_IAS=1 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)"
$M gki_defconfig
scripts/kconfig/merge_config.sh -m -O out out/.config arch/arm64/configs/vendor/parrot_GKI.config
$M olddefconfig
$M Image modules
```

Every module is built from this tree; none of the stock 5.10 modules
load on 6.1.

## Status

Proven on the device (2026-10-10, debug builds):
- the bootloader accepts our boot.img + vendor_boot.img and picks our dtb
  (flash `boot_a` / `vendor_boot_a` explicitly: ruan's bootloader doesn't
  report has-slot for vendor_boot, so `fastboot flash vendor_boot` misses)
- the kernel boots through all built-in initcalls and runs the ramdisk's
  init (ruan/dbgshell/init)
- the debug shell image (ruan/dbgshell, mkdbgshell.sh) loads its 81
  modules with the APSS watchdog (qcom_soc_wdt) early and stays alive,
  logo on, no reset; the USB serial gadget doesn't show up on the PC yet

Careful: after a few failed boots ruan's bootloader marks slot A
unbootable and boots slot B (the old stock HyperOS). Run
`fastboot --set-active=a` before every test and check
`fastboot getvar current-slot` afterwards; results from slot B boots
are meaningless. Several tests on 2026-10-10 hit this.

pstore: the stock recovery kernel (5.10.246 prebuilt) doesn't register
ramoops; our 5.10.269 build didn't either when booted into recovery
(caramel did in a full Android boot). Reading the 6.1 log through
ramoops still needs a reader that registers it.

ruan gives a bring-up almost no feedback: the panel backlight goes off
when the kernel starts, pstore keeps nothing across reboots (mtdoops
shares the ramoops region), the bootloader ignores the IMEM restart
reason and EDL entry is refused. Signals that work: PSCI SYSTEM_OFF
(dark ~30s, then ruan powers back on) against a watchdog reset (~20s),
see init/ruan_probe.c and the dbg_stage values in ruan/dbgshell/init.

Next: find out why the ACM gadget doesn't enumerate (UDC name, dwc3
role, PHY/regulator probe), or a real log channel. Build the USB device stack (dwc3-msm, its
PHYs, gcc/rpmh clocks, rpmh regulators, interconnect, SMMU, pinctrl,
GDSCs) and the APSS watchdog (qcom_wdt_core + qcom_soc_wdt, which 6.1
split out) into the kernel, and use the USB serial gadget as console
(console=ttyGS0), so the kernel streams its log to the PC from early
boot.

To do after that, roughly in this order:
1. The module that powers off / the watchdog.
2. Boot to Android: display, touch, charger, Wi-Fi, audio, IPA/data,
   video, camera (out-of-tree Qualcomm modules).
3. Xiaomi modules without source (speaker amplifiers, hall sensor,
   millet, mi_memory and others): replace or drop.
