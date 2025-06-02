# JPEG-LS extension

JPEG-LS 是一种无损/有损压缩标准。目前网上的 JPEG-LS 实现均为 [**JPEG-LS baseline (ITU-T T.87)**](https://www.itu.int/rec/T-REC-T.87/en) ，而没有 [**JPEG-LS extension (ITU-T T.870)**](https://www.itu.int/rec/T-REC-T.870/en) 。我按照 **ITU-T T.870** 文档编写了一个 **JPEG-LS extension** 实现。其中压缩算法部分基本遵循 **ITU-T T.870** 文档，压缩率高于 **JPEG-LS baseline** (压缩率评估结果见 [Image-Compression-Benchmark](https://github.com/WangXuan95/Image-Compression-Benchmark) )。此外，文件 header 并不遵循标准 (不过这也不影响压缩率)，因为我的目的并不是编写一个完全兼容的标准代码，而只是为了学习和复现 JPEG-LS extension 的算法原理。

当前，**本代码只支持 8-bit 灰度图像的无损压缩 (near=0)** 或者**有损压缩 (near>0)** 。

目前，代码已经合入 ImCvt 项目，请前往 [**ImCvt项目**](https://github.com/WangXuan95/ImCvt) 获取代码。

