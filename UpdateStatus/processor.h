#pragma once
#include "CManager.h"
#include <DBAPI.h>
#include <plog/vnlog.h>
#include <winconfig/winconfig.h>

class Processor
{
public:
	Processor();
	~Processor();

public:
	struct Status
	{
		string			mt4_login_id = "";
		string			acc_status = "";
	};
	vector<Status>		status;

private:
	string				mt4Server;
	int					mt4ManagerAcc;
	string				mt4ManagerPassword;
	string				db_driver = "";
	string				db_server = "";
	string				db_database = "";
	string				db_user = "";
	string				db_password = "";
	string				db_logname = "";

	DBAPI				dbs;
	SQLDATA				data;

public:
	int					ConnectDataBase();
	void				UpdateStatus();

private:
	int					ConnectManager();
	int				GetStutasDatabase(string db);
	string				GetIpByName(string hostname);
	int					InsertLogs(string app, int res, string comment, string db);

};

extern Processor processor;