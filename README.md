 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg)

# JPEG-LS extension

JPEG-LS 是一种无损/有损压缩标准。目前网上的 JPEG-LS 实现均为 **JPEG-LS baseline (ITU-T T.87)** [2]，而没有 **JPEG-LS extension (ITU-T T.870)** [1]。本库是我按照 **ITU-T T.870** 文档编写的实现：

- 图像压缩流部分基本遵循 **ITU-T T.870** 文档，压缩率大于 **JPEG-LS baseline (ITU-T T.87)**。相关评估详见 [Image-Compression-Benchmark](https://github.com/WangXuan95/Image-Compression-Benchmark)
- 文件头并不遵循文档！！因此并不兼容标准。
- 本库的目的仅仅是复现 JPEG-LS extension，用于学习和比较。
- 本库**不再继续维护**。本人目前在维护另一个更好的图像压缩器： [NBLI: a fast, better lossless image format](https://github.com/WangXuan95/NBLI) [4]

　

# 开发进度

- [x] 灰度 8 bit 图像无损编码/解码
- [x] 灰度 8 bit 图像有损编码/解码
- [ ] 彩色图像编码/解码：尚未计划

　

# 代码说明

代码文件在目录 src 中。包括 2 个文件：

- `JLSx.c` : 实现了 JPEG-LS extension encoder/decoder
- `JLSxMain.c` : 包含 `main` 函数的文件，是调用 `JLSx.c` 的一个示例。

　

# 编译

在本目录里运行命令产生可执行文件 JLSx.exe 。

```bash
gcc src\*.c -o JLSx -O3 -Wall
```

　

# 运行

### Windows (命令行)

用以下命令把图像文件 `1.pgm` 压缩为 `1.jlsxn`  。其中 `<near>` 值可以取非负整数。0 代表无损，≥1 代表有损，越大则压缩率越高，图像质量越差

```bash
JLSx.exe test.pgm test.jlsxn <near>
```

用以下命令把图像文件 `1.jlsxn`  解压为 `1.pgm` 。

```
JLSx.exe test.jlsxn x.pgm
```

> :warning: `.pgm` 是一种非压缩的灰度图像文件格式，详见 https://netpbm.sourceforge.net/doc/pgm.html#index

　

# 参考资料

[1] JPEG-LS extension ITU-T T.870 : https://www.itu.int/rec/T-REC-T.870/en 

[2] JPEG-LS baseline ITU-T T.87 : https://www.itu.int/rec/T-REC-T.87/en 

[3] An implementation of JPEG-LS baseline ITU-T T.87 encoder: https://github.com/WangXuan95/ImCvt

[4] NBLI: a fast, better lossless compression format: https://github.com/WangXuan95/NBLI
