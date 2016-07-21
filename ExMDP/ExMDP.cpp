// ExMDP.cpp : 定义 DLL 应用程序的导出函数。
//
#include "stdafx.h"
#include "ExMDP.h"
#include "Initiator.h"

MDP::Initiator* p = NULL;

EXMDP_API int StartEngine( ConfigStruct* configStruct, Application* application )
{
	if (configStruct == NULL)
		return -1;
	if (application == NULL)
	{
		strcpy_s(configStruct->errorInfo, "Param Application* is NULL");
		return -1;
	}

	try
	{
		if (p == NULL)
			p = new MDP::Initiator;
		else
			throw MDP::RuntimeError("[StartEngine]: failed. Engine already started!");
		p->start(configStruct, application);
	}
	catch ( std::exception& e )
	{
		strcpy_s(configStruct->errorInfo, e.what());
		return -1;
	}

	return 0;
}

EXMDP_API int StopEngine()
{
	if (p)
	{
		p->stop();
		delete p;
		p = NULL;
		return 0;
	}
	return -1;
}

