// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//
#define DEBUG
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



#define WIN32_LEAN_AND_MEAN             // �� Windows ͷ���ų�����ʹ�õ�����
// Windows ͷ�ļ�: 
#include <windows.h>
#include<io.h>
#include<vector>
typedef long sampleIndex;//Ū�úܴ����Բ�������



// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
