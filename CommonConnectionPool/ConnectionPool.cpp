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

//������
std::shared_ptr<CConnection> CConnectionPool::GetConnection()
{
	std::unique_lock<std::mutex> lock(m_queueMutex);
	while(m_connectionQueue.empty()) {
		if(std::cv_status::timeout == m_CV.wait_for(lock, std::chrono::milliseconds(m_connectionTimeout)))
			if (m_connectionQueue.empty()) {
				OutputDebugStringA("��ȡ���ӳ�ʱ����������ȡʧ��");
				return nullptr;
			}
	}

	std::shared_ptr<CConnection> sp(m_connectionQueue.front(),
		[&](CConnection* pcon) {//�ͷŷ�ʽ
			//�̰߳�ȫ
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_connectionQueue.push(pcon);
		}
	);
	m_connectionQueue.pop();
	m_CV.notify_all();//�����������Ժ�֪ͨ�������̼߳��һ�£��������Ϊ�գ��Ͻ�����
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

//������
void CConnectionPool::ProduceConnectionTask()
{
	while (true)
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		while (!m_connectionQueue.empty())
			m_CV.wait(lock);//���в��գ��˴������߳̽���ȴ��׶�
		
		//��������û�дﵽ���ޣ����������µ�����
		if (m_connectionCount < m_maxSize) {
			CConnection* p = new CConnection;
			if (p->connect(m_ip, m_port, m_userName, m_passWord, m_DBName)) {
				p->refreshAliveTime();
				m_connectionQueue.push(p);
				m_connectionCount++;
			}
			else delete p;
		}
		//֪ͨ�������̣߳���������������
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
	//����������
	if (LoadConfigFile() == false) {
		OutputDebugStringA("CConnectionPool����������ʧ��");
		return;
	}
	//������ʼ����������
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
	//����һ���µ��̣߳���Ϊ���ӵ�������
	std::thread produce(std::bind(&CConnectionPool::ProduceConnectionTask, this));
	produce.detach();
	//����һ���µ��̣߳�ɨ�賬ʱ�����ӣ��������ͷ�
	std::thread scanner(std::bind(&CConnectionPool::ScannerConnectionTask, this));
	scanner.detach();
}
CConnectionPool::deobj::deobj()
{
	CConnectionPool::m_object = new CConnectionPool();
}

CConnectionPool::deobj::~deobj()
{	//ɾ������
	if (CConnectionPool::m_object != nullptr)
	{
		delete CConnectionPool::m_object;
		CConnectionPool::m_object = nullptr;
	}
}