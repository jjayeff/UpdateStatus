// UpdateStatus.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "stdafx.h"
#include "processor.h"

Processor processor;

int _tmain(int argc, _TCHAR* argv[])
{
	//-------------------------------- Run -------------------------------
	if (processor.ConnectDataBase())
		return 1;

	processor.UpdateStatus();
}
