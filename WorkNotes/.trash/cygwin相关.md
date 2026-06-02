---
banner: ../images/Pasted image 20260128113035.png
---
### 一、添加路径
1.用加密盘打开cwgwin会提示 'ls' 等命令找不到
![](../assets/Pasted%20image%2020260128092534.png)

原因：
Cygwin 安装不完整 （可能是因为加密原因）
PATH 被错误覆盖：当前 shell 的 PATH 环境变量被清空或错误设置，导致找不到 /usr/bin/ls。

需要设置路径：

`export PATH="/usr/local/bin:/usr/bin:/bin:/cygdrive/c/Windows/system32"`
`echo 'export PATH="/usr/local/bin:/usr/bin:/bin:$PATH"' >> ~/.bashrc`
`source ~/.bashrc`

### 二、修复脚本
提示类似：
`'\r': command not found` 时可能是出现Windows换行符 CLF 需要修复
`dos2unix 报错脚本`


### 三、configure编译
1.有的第三方库文件中有配置好的configure，有的没有 则需要用autoreconf工具生成
`autoreconf -vif`
遇到：
![](../assets/Pasted%20image%2020260128102152.png)
说明 Cygwin 的 /tmp 目录权限问题，导致 autoreconf 无法创建/删除临时文件 ； 需要创建临时文件
`export TMPDIR="$HOME/tmp"`
`mkdir -p "$TMPDIR"`

### 五、proto编译
cygwin编译 proto 会在目标路径下生成proto文件夹
build_x86/src/proto

编译prota.a时 会报错：
![](../assets/Pasted%20image%2020260129152003.png)
是因为proto 源码中 Protobuf 的 protoc 编译器代码使用了 Linux 特有的 /proc/self/exe，没有对cygwin平台进行兼容，需要修改报错.cc
![](../assets/Pasted%20image%2020260129152244.png)
包含头文件：
```
#ifdef __CYGWIN__
#include <unistd.h>
#include <limits.h>
#endif
```

### 四、编译planning 报错
1、 报错：
` error: ‘uint32_t’ in namespace ‘std’ does not name a type; did you mean ‘wint_t’?`
`16 |   virtual double Evaluate(const std::uint32_t order,`
```
是因为跨平台编译时编译器在 std 命名空间中找不到 uint32_t 类型。这是因为 uint32_t 并不属于 std 命名空间，而是定义在 <cstdint>（C++11 起）或 <stdint.h>（C 风格）中，且位于全局命名空间。 需要在报错文件中包含：#include <cstdint>
```

### 五、涉及的其他三方库
1.Osqp 也需要重新编译一下
由于下载的osqp源码 其子模块无法更新，因此还需要下载其子模块 `QDLDL` 复制到osqp下
![](../assets/Pasted%20image%2020260130105855.png)
```
wget https://github.com/osqp/qdldl/archive/v0.1.5.tar.gz
tar -xzf v0.1.5.tar.gz
cp -r qdldl-0.1.5/* /usr/src/osqp-0.5.0/lin_sys/direct/qdldl/qdldl_sources/
ls /usr/src/osqp-0.5.0/lin_sys/direct/qdldl/qdldl_sources/CMakeLists.txt
```

2.Zmq
czmq需要先编译 zmplib
编译zmqlib时可能会遇到，是因为Cygwin平台中 string.h 已经声明了strlcpy 和strnlen 但ZeroMQ 源码编译时会重新声明，导致冲突
![](../assets/Pasted%20image%2020260130134555.png)
需要修改源码
在`compat.hpp` 开始添加Cygwin检测
```
#ifdef __CYGWIN__
    #define HAVE_STRLCPY 1
    #define HAVE_STRNLEN 1
#endif

// ... 原有代码 ...
```

并注释冲突函数声明
```
#ifndef __CYGWIN__
static inline size_t strlcpy (char *dest_, const char *src_, const size_t dest_size_)
{
    // ... 实现 ...
}
#endif

#ifndef __CYGWIN__
static inline size_t strnlen (const char *s, size_t len)
{
    // ... 实现 ...
}
#endif
```

Ubuntu中不会报错：
```
Ubuntu 的 glibc 没有 strlcpy（这是 BSD 函数）
Cygwin 的 newlib 实现了 strlcpy 和 strnlen
ZeroMQ 的兼容层与 Cygwin 冲突
```

3.boost
编译软件链接时会报错：
```
gwin/bin/ld: CMakeFiles/ttePlanning.dir/data_exchange/data_collector.cpp.o:data_collector:(.text+0x68c9): undefined reference to `boost::archive::archive_exception::archive_exception(boost::archive::archive_exception::_exception_code, char const*, char const*)'
```

是因为Cygwin中的 window.h 又`_exception_code`相关定义或者宏，在进行boost链接时使用的是 `exception_code`，宏被污染，且宏污染发生在「Boost 头文件被解析之前」，可能是`#include <windows.h>` 和`#include <winnt.h>`，在Cygwin环境下编译时，编译时有些定义确实需要包含这两个头文件.. 目前还没找到合适的方法解决
已经试过：1、修改boost源码 改为_excetption_code 但是由于是发生在boost解析前的 因此无用
2、在相关头文件调用前取消相关宏 
```
#ifdef exception_code 
#undef exception_code 
#endif
```
3、`CMakeLists.txt` 中添加`add_compile_options(-Dexception_code=exception_code)`
4、编译Boost 添加取消exception_code 宏定义 `Uexception_code`
```
./b2 \
  threading=multi \
  -j14 \
  link=static \
  runtime-link=static \
  toolset=gcc \
  cxxflags="-Uexception_code -std=c++14" \
  --prefix=/usr/lib/boost_1.66 \
  --with-serialization \
  --with-thread \
  --with-filesystem \
  --with-atomic \
  install
```
目前看方法2是可以的 但是需要把所有涉及调用boost_xxx.hpp的.h .cpp 等文件前都要添加 

```
$ nm -C ./lib/libboost_serialization.a | c++filt | grep "archive_exception::archive_exception" | head -2
                 U boost::archive::archive_exception::archive_exception(boost::archive::archive_exception::exception_code, char const*, char const*)
                 U boost::archive::archive_exception::archive_exception(boost::archive::archive_exception const&)

```

上述说明库没问题，是项目被污染了 所以用方法2

```
#ifdef exception_code 
#undef exception_code 
#endif
```
boost 爆炸的位置
```
/cygdrive/r/2818/src/common/json_parse.cpp:10:2: error: #error "exception_code defined BEFORE boost include1"
   10 | #error "exception_code defined BEFORE boost include1"
      |  ^~~~~
[ 19%] Building CXX object src/CMakeFiles/ttePlanning.dir/external_simulate/chassis.cpp.o
[ 20%] Building CXX object src/CMakeFiles/ttePlanning.dir/external_simulate/localization.cpp.o
In file included from /cygdrive/r/2818/src/common/common_type.h:6,
                 from /cygdrive/r/2818/src/external_simulate/localization.h:14,
                 from /cygdrive/r/2818/src/external_simulate/localization.cpp:12:
/cygdrive/r/2818/src/proto/pnc_point.pb.h:17:2: error: #error "exception_code defined BEFORE boost include6"
   17 | #error "exception_code defined BEFORE boost include6"
      |  ^~~~~
[ 20%] Building CXX object src/CMakeFiles/ttePlanning.dir/external_simulate/prediction_obstacle.cpp.o
In file included from /cygdrive/r/2818/src/common/common_type.h:6,
                 from /cygdrive/r/2818/src/external_simulate/chassis.h:16,
                 from /cygdrive/r/2818/src/external_simulate/chassis.cpp:12:
/cygdrive/r/2818/src/proto/pnc_point.pb.h:17:2: error: #error "exception_code defined BEFORE boost include6"
   17 | #error "exception_code defined BEFORE boost include6"
      |  ^~~~~
[ 20%] Building CXX object src/CMakeFiles/ttePlanning.dir/common/xml_demo.cpp.o
[ 21%] Building CXX object src/CMakeFiles/ttePlanning.dir/on_lane_planning.cpp.o
In file included from /cygdrive/r/2818/src/common/common_type.h:6,
                 from /cygdrive/r/2818/src/external_simulate/prediction_obstacle.h:20,
                 from /cygdrive/r/2818/src/external_simulate/prediction_obstacle.cpp:12:
/cygdrive/r/2818/src/proto/pnc_point.pb.h:17:2: error: #error "exception_code defined BEFORE boost include6"
   17 | #error "exception_code defined BEFORE boost include6"
      |  ^~~~~
make[2]: *** [src/CMakeFiles/ttePlanning.dir/build.make:373: src/CMakeFiles/ttePlanning.dir/common/json_parse.cpp.o] Error 1
make[2]: *** Waiting for unfinished jobs....
In file included from /cygdrive/r/2818/src/proto/planning.pb.h:17,
                 from /cygdrive/r/2818/src/common/adc_trajectory.h:10,
                 from /cygdrive/r/2818/src/common/frame.h:7,
                 from /cygdrive/r/2818/src/tasks/tasks_base/task.h:16,
                 from /cygdrive/r/2818/src/scenarios/stage.h:19,
                 from /cygdrive/r/2818/src/scenarios/scenario.h:14,
                 from /cygdrive/r/2818/src/scenarios/scenario_manager.h:18,
                 from /cygdrive/r/2818/src/planner/public_road_planner.h:15,
                 from /cygdrive/r/2818/src/on_lane_planning.h:14,
                 from /cygdrive/r/2818/src/on_lane_planning.cpp:12:
/cygdrive/r/2818/src/proto/pnc_point.pb.h:17:2: error: #error "exception_code defined BEFORE boost include6"
   17 | #error "exception_code defined BEFORE boost include6"
      |  ^~~~~
make[2]: *** [src/CMakeFiles/ttePlanning.dir/build.make:415: src/CMakeFiles/ttePlanning.dir/external_simulate/localization.cpp.o] Error 1
make[2]: *** [src/CMakeFiles/ttePlanning.dir/build.make:401: src/CMakeFiles/ttePlanning.dir/external_simulate/chassis.cpp.o] Error 1
In file included from /cygdrive/r/2818/src/common/speed/st_boundary.h:15,
                 from /cygdrive/r/2818/src/common/obstacle.h:10,
                 from /cygdrive/r/2818/src/common/open_space_info.h:10,
                 from /cygdrive/r/2818/src/external_simulate/routing.h:20,
                 from /cygdrive/r/2818/src/common/local_view.h:7,
                 from /cygdrive/r/2818/src/common/frame.h:8:
/cygdrive/r/2818/src/common/speed/st_point.h:10:2: error: #error "exception_code defined BEFORE boost include4"
   10 | #error "exception_code defined BEFORE boost include4"
```

### 六、IPOPT库编译

| 库名                        | 作用            | 典型来源          |
| ------------------------- | ------------- | ------------- |
| `ipopt`                   | 非线性规划（NLP）求解器 | COIN-OR       |
| `coinblas` / `coinlapack` | 线性代数          | BLAS / LAPACK |
| `coinmumps`               | 稀疏矩阵求解        | MUMPS         |
| `coinhsl`                 | 商业线性代数库       | HSL（需授权）      |
| `gfortran`                | Fortran 运行时   | GCC Fortran   |


编译ipopt库需要依次编译：
#### 主要流程：
gfrotran->blas->lapack->mumps->ipopt (还需要HSL 该库可能商用)
#### BLAS编译
blas 编译简单一点 生成 编译成功会生成`blas_LINUX.a`
下载源码：
```cd /usr/src
curl -LO https://netlib.org/blas/blas-3.10.0.tgz
tar xf blas-3.10.0.tgz
cd BLAS-3.10.0
```

编译（使用 gfortran）

```bash
make
```

如果成功，会生成：

```text
blas_LINUX.a
```

---

安装到 Cygwin 系统路径

```bash
cp blas_LINUX.a /usr/lib/libblas.a （后面 LAPACK / IPOPT 会链接它）
```


libblas.a编译ok：
![](../assets/Pasted%20image%2020260128171746.png)

#### LAPACK编译
编译 LAPACK 会麻烦一点

下载源码
```bash
cd /usr/src
curl -LO https://netlib.org/lapack/lapack-3.10.1.tar.gz
tar xf lapack-3.10.1.tar.gz
cd lapack-3.10.1
```
修改配置

```bash
cp make.inc.example make.inc
```


```bash
nano make.inc
```


修改后配置如下：
```
####################################################################
#  LAPACK make include file.                                       #
####################################################################

SHELL = /bin/sh

#  CC is the C compiler, normally invoked with options CFLAGS.
#
CC = gcc
CFLAGS = -O3

#  Modify the FC and FFLAGS definitions to the desired compiler
#  and desired compiler options for your machine.  NOOPT refers to
#  the compiler options desired when NO OPTIMIZATION is selected.
#
#  Note: During a regular execution, LAPACK might create NaN and Inf
#  and handle these quantities appropriately. As a consequence, one
#  should not compile LAPACK with flags such as -ffpe-trap=overflow.
#
FC = gfortran
FFLAGS = -O2
LOADOPTS = -O2
FFLAGS_DRV = $(FFLAGS)
FFLAGS_NOOPT = -O0 -frecursive

#  Define LDFLAGS to the desired linker options for your machine.
#
LDFLAGS =

#  The archiver and the flag(s) to use when building an archive
#  (library).  If your system has no ranlib, set RANLIB = echo.
#
AR = ar
ARFLAGS = cr
RANLIB = ranlib

#  Timer for the SECOND and DSECND routines
#
#  Default:  SECOND and DSECND will use a call to the
#  EXTERNAL FUNCTION ETIME
#TIMER = EXT_ETIME
#  For RS6K:  SECOND and DSECND will use a call to the
#  EXTERNAL FUNCTION ETIME_
#TIMER = EXT_ETIME_
#  For gfortran compiler:  SECOND and DSECND will use a call to the
#  INTERNAL FUNCTION ETIME
TIMER = INT_ETIME
#  If your Fortran compiler does not provide etime (like Nag Fortran
#  Compiler, etc...) SECOND and DSECND will use a call to the
#  INTERNAL FUNCTION CPU_TIME
#TIMER = INT_CPU_TIME
#  If none of these work, you can use the NONE value.
#  In that case, SECOND and DSECND will always return 0.
#TIMER = NONE

#  Uncomment the following line to include deprecated routines in
#  the LAPACK library.
#
#BUILD_DEPRECATED = Yes

#  LAPACKE has the interface to some routines from tmglib.
#  If LAPACKE_WITH_TMG is defined, add those routines to LAPACKE.
#
#LAPACKE_WITH_TMG = Yes

#  Location of the extended-precision BLAS (XBLAS) Fortran library
#  used for building and testing extended-precision routines.  The
#  relevant routines will be compiled and XBLAS will be linked only
#  if USEXBLAS is defined.
#
#USEXBLAS = Yes
#XBLASLIB = -lxblas

#  The location of the libraries to which you will link.  (The
#  machine-specific, optimized BLAS library should be used whenever
#  possible.)
#
#BLASLIB      = $(TOPSRCDIR)/librefblas.a
BLASLIB      = /usr/lib/libblas.a
CBLASLIB     = $(TOPSRCDIR)/libcblas.a
LAPACKLIB    = $(TOPSRCDIR)/liblapack.a
TMGLIB       = $(TOPSRCDIR)/libtmglib.a
LAPACKELIB   = $(TOPSRCDIR)/liblapacke.a

#  DOCUMENTATION DIRECTORY
# If you generate html pages (make html), documentation will be placed in $(DOCSDIR)/explore-html
# If you generate man pages (make man), documentation will be placed in $(DOCSDIR)/man
DOCSDIR       = $(TOPSRCDIR)/DOCS

```

直接make 可能会报错， 可能是由于CFLAG和FFLAGS 包含了 cygwin不支持的选项：
```
编译lapack：$ make
make -C INSTALL run
make[1]: 进入目录“/usr/src/lapack-3.10.1/INSTALL”
gfortran -O2 -fno-lto -fno-use-linker-plugin  -o testlsame lsame.o lsametst.o
/usr/lib/gcc/x86_64-pc-cygwin/7.4.0/../../../../x86_64-pc-cygwin/bin/ld: -f may not be used without -shared
make[1]: *** [Makefile:8：testlsame] 错误 1
make[1]: 离开目录“/usr/src/lapack-3.10.1/INSTALL”
make: *** [Makefile:55：lapack_install] 错误 
```
修复后不运行测试 只编译库 `make lapacklib `   编译时间很久

lapacklib 已经编译ok：
![](../assets/Pasted%20image%2020260128171635.png)
编译后的库 需要复制到调用位置并改名
```
cp liblapack.a libcoinlapack.a
cp libblas.a libcoinblas.a
```

并在根目录cmakelists中添加链接位置
```
link_directories(
	...
    ${CMAKE_SOURCE_DIR}/third_party/lapack
)
```


#### 七、SOMEIP
Cygwin 本质是：
Windows + cygwin1.dll 模拟 POSIX
cygwin下：没有 glibc、 没有 ELF TLS、 pthread 是“翻译层”、 socket / poll / epoll 语义不完整、 credentials、namespace、capability 全缺
但是someip需要：
使用 以太网 + TCP/UDP + SOME/IP
依赖：
高精度定时器
线程调度
socket 选项
credentials / SO_PASSCRED
多播、TTL、QoS
稳定运行 7×24，不允许偶发异常

所以someip无法在cygwin下使用..
![](../assets/Pasted%20image%2020260211120049.png)


###  编过了
![](../assets/Pasted%20image%2020260213145348.png)

