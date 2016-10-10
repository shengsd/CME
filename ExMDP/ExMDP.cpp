// ExMDP.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include "ExMDP.h"
#include "Initiator.h"

MDP::Initiator p;

EXMDP_API int StartEngine( ConfigStruct* configStruct, Application* application )
{
	return p.start(configStruct, application);
}

EXMDP_API int StopEngine()
{
	return p.stop();
}

