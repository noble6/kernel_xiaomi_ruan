#!/bin/bash
# Build the experimental 6.1 parrot kernel: work/build61.sh [make targets]
cd /mnt/directstorage/kernel-src
K=$PWD/uke61; O=$PWD/out-61
export PATH=$PWD/clang-r487747c/bin:$PATH
M="make -C $K O=$O ARCH=arm64 LLVM=1 LLVM_IAS=1 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)"
$M gki_defconfig && $K/scripts/kconfig/merge_config.sh -m -O $O $O/.config $K/arch/arm64/configs/vendor/parrot_GKI.config && $M olddefconfig && $M -k ${@:-Image modules}
