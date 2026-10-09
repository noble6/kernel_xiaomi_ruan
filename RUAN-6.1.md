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

Done:
- parrot clock controllers, pinctrl, interconnect and UFS PHY
- parrot GKI config fragment
- Image and in-tree modules build

To do, roughly in this order:
1. Device tree: the stock dtb uses 5.10 compatibles that 6.1 drivers no
   longer match (`qcom,qupv3-geni-se`, `qcom,parrot-rpmh-clk`, the
   `qcom,parrot-*-pas` remoteprocs, `qcom,cpufreq-hw-epss` and others).
   Add them to the drivers or adapt the dtb.
2. First boot to adb, debugged through the pstore/ramoops log.
3. Out-of-tree Qualcomm modules: display, GPU (KGSL), touch, charger,
   Wi-Fi, audio, IPA/data, video, camera.
4. Xiaomi modules without source (speaker amplifiers, hall sensor,
   millet, mi_memory and others): replace or drop.
