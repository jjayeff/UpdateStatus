#include "pch.h"
#include "stdafx.h"
#include "Processor.h"

CManager		manager;
Configuration	config;
LogClass		vnLog;

//+------------------------------------------------------------------+
//| Constructor                                                      |
//+------------------------------------------------------------------+
Processor::Processor() {
	//---Setting configuration
	config.setConfig("UpdateStatus.ini");

	string appPath = config.getAbsolutePath();
	string log_path = appPath + "logs";

	config.setValue("Application", "MT4Server", "edge01-mirrorlive.toptrader.co.th");
	config.setValue("Application", "ManagerAccount", "11");
	config.setValue("Application", "ManagerPassword", "app@dmin234");
	config.setValue("Application", "LogPath", log_path);

	config.setValue("Database", "Driver", "SQL Server Native Client 11.0");
	config.setValue("Database", "Server", "172.17.1.43");
	config.setValue("Database", "Database", "acc_info");
	config.setValue("Database", "Username", "sa");
	config.setValue("Database", "Password", "P@ssw0rd");
	config.setValue("Database", "LogName", "UpdateStatus");

	mt4Server = config.getValueString("Application", "MT4Server");
	mt4ManagerAcc = config.getValueInt("Application", "ManagerAccount");
	mt4ManagerPassword = config.getValueString("Application", "ManagerPassword");
	db_driver = config.getValueString("Database", "Driver");
	db_server = GetIpByName(config.getValueString("Database", "Server"));
	db_user = config.getValueString("Database", "Username");
	db_password = config.getValueString("Database", "Password");
	db_logname = config.getValueString("Database", "LogName");

	vnLog.InitialLog(config.getValueString("Application", "LogPath"), "UpdateStatus", 10, true);
}
//+------------------------------------------------------------------+
//| Destructor                                                       |
//+------------------------------------------------------------------+
Processor::~Processor() {
}
//+------------------------------------------------------------------+
//| Database Function                                                |
//+------------------------------------------------------------------+
int Processor::ConnectDataBase() {
	// Connect Datebase
	if (!dbs.connect(db_driver, db_server, db_user, db_password))
	{
		LOGE << "!Database connect fail";
		dbs.commit();
		return 1;
	}
	else
	{
		LOGI << "Database connected : " << db_server << ", Driver : " << db_driver;
		return 0;
	}
}
int Processor::GetStutasDatabase(string db) {
	if (!dbs.isConnected())
	{
		LOGW << "Database disconnect! Try reconnect!";
		dbs.connect();
	}

	char cmd_temp[1024] = { 0 };
	sprintf_s(cmd_temp, "SELECT mt4_login_id,acc_status FROM %s.dbo.acc_info", db.c_str());

	if (!dbs.execute(cmd_temp))
	{
		LOGE << "!Excute database fail";
		return 1;
	}
	data = dbs.getSQLData();
	while (data.FetchNext())
	{
		Status tmp;
		tmp.mt4_login_id = data.GetField("mt4_login_id");
		string status_symbol = data.GetField("acc_status");
		if(status_symbol == "A")
			tmp.acc_status = "Enable";
		else if (status_symbol == "M")
			tmp.acc_status = "Call Margin";
		else if (status_symbol == "T")
			tmp.acc_status = "Disable Trade";
		else if (status_symbol == "F")
			tmp.acc_status = "Call Force";
		else if (status_symbol == "S")
			tmp.acc_status = "Disable";
		else if (status_symbol == "D")
			tmp.acc_status = "Delete";
		else if (status_symbol == "C")
			tmp.acc_status = "Close Only";

		status.push_back(tmp);
	}
	return 0;
}
int Processor::InsertLogs(string app, int res, string comment, string db) {
	if (!dbs.isConnected())
	{
		LOGE << "Database disconnect! Try reconnect!";
		dbs.connect();
	}

	char cmd_temp[512];
	sprintf_s(cmd_temp, "INSERT INTO %s.dbo.app_schedule_log VALUES('%s', GETDATE(), '%d', '%s');", db.c_str(), app.c_str(), res, comment.c_str());

	if (!dbs.execute(cmd_temp))
	{
		LOGE << "!Excute database fail";
		return 0;
	}

	LOGI << "InsertLog Success!!";
	return 1;
}
//+------------------------------------------------------------------+
//| MT4 Manager Function                                             |
//+------------------------------------------------------------------+
string Processor::GetIpByName(string hostname)
{
	string ans = hostname;
	WSADATA wsaData;
	int iResult;
	DWORD dwError;

	struct hostent* remoteHost;
	struct in_addr addr;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		return hostname;
	}

	remoteHost = gethostbyname(ans.c_str());

	if (remoteHost == NULL)
	{
		dwError = WSAGetLastError();
		if (dwError != 0)
		{
			if (dwError == WSAHOST_NOT_FOUND)
			{
				return ans;
			}
			else if (dwError == WSANO_DATA)
			{
				return ans;
			}
			else
			{
				return ans;
			}
		}
	}
	else
	{
		if (remoteHost->h_addrtype == AF_INET)
		{
			addr.s_addr = *(u_long *)remoteHost->h_addr_list[0];
			ans = inet_ntoa(addr);
		}
	}
	return ans;
}
int Processor::ConnectManager()
{
	int   res = RET_ERROR;
	string server = GetIpByName(mt4Server);
	int id = mt4ManagerAcc;
	string pwd = mt4ManagerPassword;

	if (!manager.IsValid()) return res;

	if ((res = manager->Connect(server.c_str())) != RET_OK || (res = manager->Login(id, pwd.c_str())) != RET_OK)
	{
		LOGE << "Connect to " << server << " as '" << id << "' failed (" << manager->ErrorDescription(res) << ")";
		return res;
	}
	LOGI << "Connect to " << server << " as '" << id << "' successfully";

	return res;
}
//+------------------------------------------------------------------+
//| Main Function                                                    |
//+------------------------------------------------------------------+
void Processor::UpdateStatus(string db_database)
{
	try
	{
		// Get status from datebase acc_info
		if (GetStutasDatabase(db_database)) {
			string log = "Connecting data fail";
			LOGE << log;
			InsertLogs(db_logname + "#1", 0, log, db_database);
			return;
		}


		//----------------------------------------
		int res = RET_ERROR;
		if ((res = ConnectManager()) != RET_OK)
		{
			string log = "Connect failed : " + string(manager->ErrorDescription(res));
			LOGE << log;
			InsertLogs(db_logname + "#1", 0, log, db_database);
			return;
		}
		int total_id = 0;
		vector< int > arr;
		GroupCommandInfo conGroup = { 0 };
		conGroup.command = GROUP_DELETE;

		// Get all data account
		UserRecord * allAccount = manager->UsersRequest(&total_id);
		// For-loop for make array will delete account when group == input
		for (int i = 0; i < total_id; i++)
			for (auto value : status)
			{
				UserRecord tmp = allAccount[i];
				if (value.mt4_login_id == to_string(allAccount[i].login)) {
					// Update status
					strcpy_s(tmp.status, value.acc_status.c_str());
					// Manager update status
					res = RET_ERROR;
					if ((res = manager->UserRecordUpdate(&tmp)) != RET_OK)
					{
						string log = "Connect failed : " + string(manager->ErrorDescription(res));
						LOGE << log;
						InsertLogs(db_logname + "#1", 0, log, db_database);
						return;
					}
					else {
						LOGI << "OK Update List of Account " + value.mt4_login_id;
					}
					break;
				}
			}
		
		// Clear memory
		manager->MemFree(allAccount);
		LOGD << status.size();
		status.clear();
		LOGD << status.size();


		// Save log
		string log = "Update Status Success !!";
		LOGI << log;
		InsertLogs(db_logname + "#1", 1, log, db_database);

	}
	catch (exception& e)
	{
		LOGE << "exception : " << e.what();
	}
}