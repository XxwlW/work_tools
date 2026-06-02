方案一：

Dokcer 

将ubuntu下的工程打包为docker镜像 在windows下直接run

GUI支持需要XServer

安装需要：WSL 、windows开启虚拟平台

方案二：

WSL + XServer

需要开启虚拟机平台、Linux的Windows子系统

方案三：

代码转为Windows

方案四：

MinGW-w64 交叉编译

依赖的第三方库需要改为windows下的三方库



linux下 的算法库需要编译为windows下的动态





CygWin：

在 Cygwin 环境里重新编译 软件 源码怎么弄呢





Linux 交叉编译：

MinGW-w64 版本较低 无法编译第三方库 需要升级

