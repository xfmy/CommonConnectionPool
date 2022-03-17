#pragma once
#include <mysql.h>
#include <string>
#include <ctime>
// 数据库操作类
class CConnection
{
public:
	CConnection();							// 初始化数据库连接

	~CConnection();								// 释放数据库连接资源

	bool			connect(std::string ip, unsigned short port, std::string user, std::string password,
		std::string dbname);					// 连接数据库

	bool			update(std::string sql);	// 更新操作 insert、delete、update

	MYSQL_RES*		query(std::string sql);		// 查询操作 select
	void			refreshAliveTime();			//刷新一下连接的起始的空闲时间点
	std::clock_t	GetAliceTime();				//返回存活的时间
private:
	MYSQL*			_conn;						// 表示和MySQL Server的一条连接
	std::clock_t	m_alivetTime;
};