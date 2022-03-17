#pragma once
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>
#include <condition_variable>
#include "Connection.h"

class CConnectionPool
{
public:
	static CConnectionPool*			GetCConnectionPoolObject();		//单例
	std::shared_ptr<CConnection>	GetConnection();	//提供外部获取连接的接口
private:
	//单列销毁
	class deobj
	{
	public:
		deobj();
		~deobj();
	};
	CConnectionPool();
	//~CConnectionPool();
	std::string				m_ip;					//mysql-ip地址
	unsigned short			m_port;					//mysql-端口号
	std::string				m_userName;				//mysql-登录用户名
	std::string				m_passWord;				//mysql-登录密码
	std::string				m_DBName;				//数据库名
	int						m_initSize;				//连接池的初始链接数量
	int						m_maxSize;				//连接池的最大连接量
	int						m_maxIdleTime;			//连接池的最大空闲时间
	int						m_connectionTimeout;	//连接池获取链接的超时时间

	std::queue<CConnection*>m_connectionQueue;		//存储mysql连接的数量
	std::mutex				m_queueMutex;			//维护队列安全的互斥锁

	static CConnectionPool*	m_object;				//单例对象
	static deobj			heleper;				//静态单例创建与销毁
	std::atomic_uint		m_connectionCount;		//记录链接所创建的总数量
	std::condition_variable m_CV;					//条件变量，用于连接生产线程和连接线程的通信
private:
	bool					LoadConfigFile();		//加载配置文件
	void					ProduceConnectionTask();//运行在独立的线程中，负责产生新的线程
	void					ScannerConnectionTask();//运行在独立的线程中，负责扫描超时的连接，并负责释放
};

