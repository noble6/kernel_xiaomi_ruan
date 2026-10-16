# Linux 6.1 for ruan (experimental)

**Boots to Android userspace, no display or USB yet.** The release
kernel for the Redmi Pad Pro 5G / POCO Pad 5G (ruan) is 5.10 on the
`axion` branch.

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

2026-10-16, ruan/mkimg61.sh images (all 285 modules in the vendor
ramdisk) on slot A: UFS comes up at 0.6 s, first stage init mounts the
partitions, SELinux goes enforcing and second stage init starts the
Android services (apexd, logd, ...). It then stays on the boot logo:
there is no 6.1 display driver yet (the Qualcomm techpack drivers aren't
built), and USB device mode doesn't work, so there's no adb.

What it took, each found from a crash log (see Logs below):
- qcom_scm: ruan's TrustZone doesn't know the multi SMC wait queue
  query; fall back to single SMC instead of failing the probe (that left
  a dangling __scm and msm_sharedmem crashed on it)
- DT (dts61.py): RPMh RSCs in the 6.1 layout (drv/channel children, the
  clients under drv), no clocks on the RSCs (they only held the display
  RSC and every NoC voting through it back until the deferred probe
  timeout, after first stage init gave up on UFS), qcom,skip-qos on the
  NoCs, qcom,iova-width on the SMMU TBUs (5.10 defaulted to 36, 6.1
  fails the TBU and with it the SMMU)
- mkimg61.sh loads minidump, the log buffer hooks and download mode
  right after the hwspinlock, so later crashes are saved

Still failing, harmless so far: PMIC SID 6 transaction (pmic-spmi 0-06),
the mnoc_hf_0 TBU (display path), several thermal and temp-alarm zones.

## Logs

pstore stays empty on ruan: the bootloader clears the ramoops memory on
every reboot, warm or cold. After a panic or a watchdog bite it saves a
Qualcomm minidump to the rawdump partition instead, with the kernel log;
a Power + Volume Down reset saves one as well (handy when the boot hangs).
Only the last dump is kept, so after a failed 6.1 boot:
1. get to fastboot holding only Volume Down when the screen goes dark
   (Power + Volume Down makes a new dump of the boot you interrupt)
2. flash the 5.10 boot_a / vendor_boot_a back, `fastboot --set-active=a`
3. boot 5.10 (with root) and run device_xiaomi_ruan's
   tools/ruan_lastlog.py: panic-kmsg.txt is the 6.1 kernel log

Careful: after a few failed boots ruan's bootloader marks slot A
unbootable and boots slot B (the old stock HyperOS). Run
`fastboot --set-active=a` before every test. Flash `boot_a` /
`vendor_boot_a` explicitly: `fastboot flash vendor_boot` misses.

## Next

1. USB device mode (dwc3-msm, PHYs) for adb, so userspace can be
   debugged live.
2. The techpack drivers, display first: msm_drm, kgsl, then audio,
   Wi-Fi, IPA/data, video, camera.
3. Xiaomi modules without source (speaker amplifiers, hall sensor,
   millet, mi_memory and others): replace or drop.
