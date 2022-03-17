#include "ConnectionPool.h"
#include <Windows.h>
#include "Connection.h"
CConnectionPool* CConnectionPool::m_object = nullptr;
CConnectionPool::deobj CConnectionPool::heleper;
#define LOADINFO "ip","port","username","password","initsize","maxsize","maxidletime","connectiontimeout","dbname"
#define LOADNUMS 9
#define KEYNAME "info"

CConnectionPool* CConnectionPool::GetCConnectionPoolObject()
{
	return m_object;
}

//消费者
std::shared_ptr<CConnection> CConnectionPool::GetConnection()
{
	std::unique_lock<std::mutex> lock(m_queueMutex);
	while(m_connectionQueue.empty()) {
		if(std::cv_status::timeout == m_CV.wait_for(lock, std::chrono::milliseconds(m_connectionTimeout)))
			if (m_connectionQueue.empty()) {
				OutputDebugStringA("获取连接超时。。。。获取失败");
				return nullptr;
			}
	}

	std::shared_ptr<CConnection> sp(m_connectionQueue.front(),
		[&](CConnection* pcon) {//释放方式
			//线程安全
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_connectionQueue.push(pcon);
		}
	);
	m_connectionQueue.pop();
	m_CV.notify_all();//消费完连接以后，通知生产者线程检查一下，如果队列为空，赶紧生产
	return sp;
}

bool CConnectionPool::LoadConfigFile()
{
	char strPath[MAX_PATH]{};
	char buf[1024]{};
	int res = 0;
	const char* valueName[LOADNUMS] = { LOADINFO };
	GetCurrentDirectoryA(MAX_PATH, strPath);
	strcat_s(strPath, MAX_PATH, "\\mysql.ini");
	for (int i = 0; i < LOADNUMS; i++)
	{
		res = GetPrivateProfileStringA(KEYNAME, valueName[i], nullptr, buf, 1024, strPath);
		if (res == 0) {
			OutputDebugString(L"Load Down File path error");
			return false;
		}
		switch (i)
		{
		case 0:
			m_ip.assign(buf);
			break;
		case 1:
			m_port = atoi(buf);
			break;
		case 2:
			m_userName.assign(buf);
			break;
		case 3:
			m_passWord.assign(buf);
			break;
		case 4:
			m_initSize = atoi(buf);
			break;
		case 5:
			m_maxSize = atoi(buf);
			break;
		case 6:
			m_maxIdleTime = atoi(buf);
		case 7:
			m_connectionTimeout = atoi(buf);
			break;
		case 8:
			m_DBName.assign(buf);
			break;
		}
		memset(buf, 0, 1024);
	}
	return true;
}

//生产者
void CConnectionPool::ProduceConnectionTask()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		while (!m_connectionQueue.empty())
			m_CV.wait(lock);//队列不空，此处生产线程进入等待阶段
		
		//连接数量没有达到上限，继续创建新的连接
		if (m_connectionCount < m_maxSize) {
			CConnection* p = new CConnection;
			if (p->connect(m_ip, m_port, m_userName, m_passWord, m_DBName)) {
				p->refreshAliveTime();
				m_connectionQueue.push(p);
				m_connectionCount++;
			}
			else delete p;
		}
		//通知消费者线程，可以消费连接了
		m_CV.notify_all();
	}
}

void CConnectionPool::ScannerConnectionTask()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(m_maxIdleTime));
		std::unique_lock<std::mutex> lock(m_queueMutex);
		while (m_connectionCount >= m_maxSize)
		{
			CConnection* p = m_connectionQueue.front();
			if (p->GetAliceTime() >= m_maxIdleTime) {
				m_connectionQueue.pop();
				m_connectionCount--;
				delete p;
			}
			else break;
		}
	}
}

CConnectionPool::CConnectionPool():
	m_port(0), m_initSize(0), m_maxSize(0), m_maxIdleTime(0), m_connectionTimeout(0) {
	//加载配置项
	if (LoadConfigFile() == false) {
		OutputDebugStringA("CConnectionPool加载配置项失败");
		return;
	}
	//创建初始数量的连接
	for (int i = 0; i < m_initSize; i++)
	{
		CConnection* p = new CConnection;
		if (p->connect(m_ip, m_port, m_userName, m_passWord, m_DBName)) {
			p->refreshAliveTime();
			m_connectionQueue.push(p);
			m_connectionCount++;
		}
		else delete p;
	}
	//启动一个新的线程，作为链接的生产者
	std::thread produce(std::bind(&CConnectionPool::ProduceConnectionTask, this));
	produce.detach();
	//启动一个新的线程，扫描超时的连接，并负责释放
	std::thread scanner(std::bind(&CConnectionPool::ScannerConnectionTask, this));
	scanner.detach();
}
CConnectionPool::deobj::deobj()
{
	CConnectionPool::m_object = new CConnectionPool();
}

CConnectionPool::deobj::~deobj()
{	//删除单列
	if (CConnectionPool::m_object != nullptr)
	{
		delete CConnectionPool::m_object;
		CConnectionPool::m_object = nullptr;
	}
}