#include "Connection.h"

CConnection::CConnection()
{
	_conn = mysql_init(nullptr);
	m_alivetTime = 0;
}

CConnection::~CConnection()
{
	if (_conn != nullptr)
		mysql_close(_conn);
}

bool CConnection::connect(std::string ip, unsigned short port, std::string user, std::string password, std::string dbname)
{
	MYSQL* p = mysql_real_connect(_conn, ip.c_str(), user.c_str(),
		password.c_str(), dbname.c_str(), port, nullptr, 0);
	return p != nullptr;
}

bool CConnection::update(std::string sql)
{
	if (mysql_query(_conn, sql.c_str()))
	{
		sql = "¸üÐÂÊ§°Ü:" + sql;
		OutputDebugStringA(sql.c_str());
		return false;
	}
	return true;
}

MYSQL_RES* CConnection::query(std::string sql)
{
	if (mysql_query(_conn, sql.c_str()))
	{
		sql = "²éÑ¯Ê§°Ü:" + sql;
		OutputDebugStringA(sql.c_str());
		return nullptr;
	}
	return mysql_use_result(_conn);
}

void CConnection::refreshAliveTime() { m_alivetTime = clock(); }

std::clock_t CConnection::GetAliceTime() { return clock() - m_alivetTime; }
