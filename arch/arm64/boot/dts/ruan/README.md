# ruan device tree

`ruan-stock-5.10.dts` is the device tree ruan boots with on HyperOS
OS3.0 (5.10): the "Parrot SoC" dtb from the stock `vendor_boot` with
ruan's overlay from `dtbo.img` ("Parrot QRD, RUAN based on SM7435",
board-id 0x3000b) applied, decompiled with dtc. It uses Qualcomm's 5.10
bindings and is kept for reference.

The 6.1 kernel needs it converted to the 6.1 bindings, e.g. the QUP
serial engines as children of a `qcom,geni-se-qup` wrapper that holds
the AHB clocks, instead of standalone nodes pointing to a
`qcom,qupv3-geni-se` wrapper.
