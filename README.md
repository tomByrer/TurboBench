TurboBench: Compressor Benchmark
================================
+ **TurboBench**
 - The only benchmark program including **LzTurbo**
 - 100% in-memory benchmark, no I/O overhead
 - Include (>50) allmost all popular, latest or fastest compressors in one compiled package 
 - **Minimum** plugins call **overhead**
 - Set one, a group or several compressors to benchmark at the command line
 - **Multiple** input files with **recursive** directories
 - Concatenate **multiple small files** into one multiblock file
 - Benchmark **multiblock** file as one large block, but each block processed separatelly
 - Avoid **cache szenario** found in other benchmarks with small files
 - Set block size, file size limit,...
 - Set number of **iterations**, number of **runs**, **benchamrks**, set max. **time** per run
 - Automatic **sort** by compressed length
 - 64 bits **Linux** and **Windows** versions (gcc 5.2/mingw64 gcc5.2)
 - Benchmarking **Entropy Coders**, **Lz77** and **BWT** compressors
 - **Text**, **html**, **csv**, **markdown** and other output formats
 - 100% C/C++, w/o inline assembly
 - Enable disable groups or individual codecs
 - No other compressor benchmark includes more codecs or offer more precision and features
 - **Transfer speed sheet** for different connections or devices: GPRS,3G,4G,DSL,Network,HDD,SSD,RAM

### Download benchmark program incl. LzTurbo:
 - [TurboBench Linux](https://sites.google.com/site/powturbo/downloads)
 - [TurboBench Windows](https://sites.google.com/site/powturbo/downloads)

### Benchmark:
CPU: Sandy bridge i7-2600k at 4.2GHz, gcc 5.2, ubuntu 15.10, single thread.
- Realistic and practical benchmark with large files
- No PURE cache benchmark

##### - Data files:
 - Comming soon...


### Plugins:
#### Compressor Lz77,Rolz,Bwt:
 - [balz v1.20](http://sourceforge.net/projects/balz) Public Domain
 - [bcm v1.0](http://sourceforge.net/projects/bcm) Public Domain
 - [Blosc v1.7.1](https://github.com/Blosc/c-blosc2) BSD license
 - [BriefLz v1.1.0](https://github.com/jibsen/brieflz) BSD like
 - [Brotli v15-11](https://github.com/google/brotli) Apache license
 - [Bzip2 v1.06](http://www.bzip.org/downloads.html) BSD like
 - [Chameleon v15-03](http://cbloomrants.blogspot.de/2015/03/03-25-15-my-chameleon.html) Public Domain
 - [Crush v1.0.0](http://sourceforge.net/projects/crush) Public Domain
 - [CSC v15-06](https://github.com/fusiyuan2010/CSC) Public domain
 - [Density v0.13.0](https://github.com/centaurean/density) BSD license
 - [Doboz v14-01-14](https://bitbucket.org/attila_afra) BSD Like
 - [FastLz v0.1.0](http://fastlz.org) BSD like
 - [Gipfeli v11.10](https://github.com/google/gipfeli) Apache license
 - [heatshrink v0.4.1](https://github.com/atomicobject/heatshrink) BSD license
 - [bsc v3.1.0](https://github.com/IlyaGrebnov/libbsc) Apache license
 - [LibLZF v1.06](http://oldhome.schmorp.de/marc/liblzf.html) BSD license
 - [LibLzg v1.0.8](https://github.com/mbitsnbites/liblzg) zlib-license
 - [Lz4 v1.7.1](https://github.com/Cyan4973/lz4) BSD license
 - [Lz5 v1.7.2](https://github.com/inikep/lz5) BSD license
 - [Lzham v1.1](https://github.com/richgel999/lzham_codec_devel) MIT license
 - [Lzma v9.35](http://7-zip.org) Public Domain
 - [LzTurbo v1.3](https://sites.google.com/site/powturbo)         
 - [Pithy v2011](https://github.com/johnezang/pithy) BSD license
 - [Shrinker v0.1/r9](https://code.google.com/p/data-shrinker) BSD license
 - [Snappy v1.1.3](https://github.com/google/snappy) Apache license
 - [Snappy-c v1.1.2/14.04](https://github.com/andikleen/snappy-c) BSD Like
 - [wfLZ v15-04](https://github.com/ShaneWF/wflz) CC0 license
 - [Yalz77 v15-09](https://github.com/ivan-tkatchev/yalz77) Public domain
 - [Yappy v.]() 
 - [zlib v1.2.8](http://zlib.net) zlib license
 - [Libzling v15-09](https://github.com/richox/libzling) BSD license
 - [Zopfli v15-05](https://code.google.com/p/zopfli) Apache license
 - [ZSTD v0.3.6](https://github.com/Cyan4973/zstd) BSD license
 - [Lzlib v1.7](http://www.nongnu.org/lzip) GPL license
 - [Lzmat v1.0](https://github.com/nemequ/lzmat) GPL license
 - [Lzo v2.09](http://www.oberhumer.com/opensource/lzo) GPL license
 - [ms-compress v15.09](https://github.com/coderforlife/ms-compress) GPL license
 - [Quicklz v1.5.1](http://www.quicklz.com) GPL license
 - [Tornado v0.6a](http://freearc.org) GPL license

#### Entropy coder:
 - [TurboANX-ANS v1.3](https://sites.google.com/site/powturbo)         
 - [TurboHF-Huffmann v1.3](https://sites.google.com/site/powturbo)         
 - [TurboRC-Range Coder v1.3](https://sites.google.com/site/powturbo)         
 - [Finite State Coder v15-05](https://github.com/skal65535/fsc) Apache license
 - [Finite State Entropy v15-11](https://github.com/Cyan4973/FiniteStateEntropy) BSD license
 - [Fast HF v2006](http://www.cipr.rpi.edu/research/SPIHT/) BSD like
 - [FastAri v15-10](https://github.com/davidcatt/FastARI) MIT license
 - [Fast AC v2006](http://www.cipr.rpi.edu/research/SPIHT/) BSD like
 - [Range Coder/J.Bonfield v15-07](ftp://ftp.sanger.ac.uk/pub/users/jkb) BSD license
 - [ANS/J.Bonfield v15-08](ftp://ftp.sanger.ac.uk/pub/users/jkb) Public Domain
 - [subotin v2000](http://ezcodesample.com/ralpha/Subbotin.txt) Public Domain
 - [zlib Huffmann v1.2.8](https://github.com/Cyan4973/FiniteStateEntropy) BSD license
 - [Polar Codes v10-07](http://www.ezcodesample.com/prefixer/prefixer_article.html) GPL license
 - [Tornado Huf v0.6a](http://freearc.org/Research.aspx) GPL license

 - [inline memcpy v.](https://github.com/powturbo/TurboBench) 
 - [library memcpy v.](https://github.com/powturbo/TurboBench) 

### Compile:
  *make*

### Testing:
##### - Groups:
  + test all fast compressor in the lz4, zlib class<br />


        ./turbobench -eFAST inputfile

##### - Codecs:

  + individual codec test<br />


        ./turbobench -elzturbo,30,31,32,39/zlib,1,5,9 inputfile

        ./turbobench -eFAST/bzip2 inputfile

### Environment:
###### OS/Compiler (64 bits):
- Linux: GNU GCC (>=4.6)
- clang (>=3.2)
- Windows: MinGW-w64

### References:

Last update: 16 NOV 2015

