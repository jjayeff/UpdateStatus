// UpdateStatus.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "stdafx.h"
#include "processor.h"

Processor processor;

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 2) {
		LOGE << "Usage: " << argv[0] << " [TFEX = 1],[Stock = 2]";
		return 1;
	}
	//-------------------------------- Run -------------------------------
	if (processor.ConnectDataBase()) {
		LOGE << "Fail connect database: ";
		return 1;
	}

	switch (stoi(argv[1]))
	{
	case 1:
		processor.UpdateStatus("acc_info");
		break;
	case 2:
		processor.UpdateStatus("acc_info_stock");
		break;
	}

}
