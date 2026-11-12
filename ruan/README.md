# Linux 6.6 for ruan (experimental)

Status: imported, not built, not booted. The release kernel for the Redmi
Pad Pro 5G / POCO Pad 5G (ruan) stays 5.10 on the `axion` branch.

## Base

Xiaomi's `annibale-w-oss` (Linux 6.6.57, March 2026) from
MiCode/Xiaomi_Kernel_OpenSource. It is Qualcomm's shared 6.6 msm tree, and
unlike the 5.10 `ruan-u-oss` release it already has parrot (SM7435) support:
`drivers/clk/qcom/*-parrot.c`, `pinctrl-parrot`, interconnect, `parrot.bzl`,
`modules.list.msm.parrot` and `arch/arm64/configs/vendor/parrot_*.config`.
Matching sources on the same branch name in MiCode:

- kernel_devicetree (parrot device trees)
- vendor_qcom_opensource_audio-kernel
- vendor_qcom_opensource_camera-kernel
- vendor_qcom_opensource_wlan_qcacld-3.0
- vendor_opensource_display-drivers

## Plan

1. Build with plain make: `ruan/build66.sh` (gki_defconfig, then
   parrot_perf.config and ruan.config).
2. Boot to Android userspace with storage, display and USB, using
   ruan's 5.10 device tree converted where the 6.6 drivers expect another
   layout (the 6.1 attempt needed that for GENI and RPMh).
3. Then port the remaining drivers one at a time: speaker amps (SIA81xx
   from public source), touch and pen, battery and charger, Wi-Fi, camera.
   Xiaomi's closed modules are left out until there is source for them.

## Testing without a ROM build

The kernel, dtb and vendor ramdisk modules go into copies of the installed
ROM's boot.img and vendor_boot.img. The ROM itself is not rebuilt. Keep the
stock boot and vendor_boot images to flash back.
