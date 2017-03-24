// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//
#define RELEASE
#pragma once
#pragma warning(disable: 4996)
#include "targetver.h"
#include "../foobar2000/SDK/foobar2000.h"
#include "../foobar2000/helpers/helpers.h"
#ifdef DEBUG
#pragma comment(lib,"..\\Debug\\pfc.lib")
#pragma comment(lib,"..\\foobar2000\\foobar2000_component_client\\Debug\\foobar2000_component_client.lib")
#pragma comment(lib,"..\\foobar2000\\SDK\\Debug\\foobar2000_SDK.lib")
#pragma comment(lib,"..\\Debug\\foobar2000_sdk_helpers.lib")
#endif // DEBUG
#ifdef RELEASE
#pragma comment(lib,"..\\Release\\pfc.lib")
#pragma comment(lib,"..\\foobar2000\\foobar2000_component_client\\Release\\foobar2000_component_client.lib")
#pragma comment(lib,"..\\foobar2000\\SDK\\Release\\foobar2000_SDK.lib")
#pragma comment(lib,"..\\Release\\foobar2000_sdk_helpers.lib")
#endif // 



#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件: 
#include <windows.h>
#include<io.h>
#include<vector>
typedef long sampleIndex;//弄得很大，所以不批量用



// TODO:  在此处引用程序需要的其他头文件
