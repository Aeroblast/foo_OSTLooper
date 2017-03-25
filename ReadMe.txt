一个foobar2000插件
=====简介=====
高二研学课题，填坑向。
买到的游戏OST音轨通常是 引入-循环部分[-循环部分]-淡出，对这类音乐搜索循环起点和终点并无缝循环播放。

====项目设置====
使用的fb2k sdk是官网的SDK-2015-08-03。
对pfc,foobar2000_component_client,foobar2000_sdk_helpers,foobar2000_SDK做项目依赖,顺序见头文件。

===主要文件====
foo_ostlooper.cpp:交互。与用户和与播放器。
LoopSeeker:一些计算。搜索循环点，画图之类的。
looper.cpp:播放。一个fb2k的decoder以提供精确到采样的循环跳转（虽说搜索部分很渣这么精确也没啥用就是了