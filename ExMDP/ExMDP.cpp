// ExMDP.cpp : ���� DLL Ӧ�ó���ĵ���������
//
#include "stdafx.h"
#include "ExMDP.h"
#include "Initiator.h"

MDP::Initiator* p = NULL;

EXMDP_API int StartEngine( ConfigStruct* configStruct, Application* application )
{
	if (!p)
	{
		p = new MDP::Initiator;
		return p->Start(configStruct, application);
	}
	else
	{
		return -1;
	}
}

EXMDP_API int StopEngine()
{
	if (p)
	{
		p->Stop();
		delete p;
		p = NULL;
		return 0;
	}
	else
	{
		return -1;
	}
}

