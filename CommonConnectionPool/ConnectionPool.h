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
	static CConnectionPool*			GetCConnectionPoolObject();		//����
	std::shared_ptr<CConnection>	GetConnection();	//�ṩ�ⲿ��ȡ���ӵĽӿ�
private:
	//��������
	class deobj
	{
	public:
		deobj();
		~deobj();
	};
	CConnectionPool();
	//~CConnectionPool();
	std::string				m_ip;					//mysql-ip��ַ
	unsigned short			m_port;					//mysql-�˿ں�
	std::string				m_userName;				//mysql-��¼�û���
	std::string				m_passWord;				//mysql-��¼����
	std::string				m_DBName;				//���ݿ���
	int						m_initSize;				//���ӳصĳ�ʼ��������
	int						m_maxSize;				//���ӳص����������
	int						m_maxIdleTime;			//���ӳص�������ʱ��
	int						m_connectionTimeout;	//���ӳػ�ȡ���ӵĳ�ʱʱ��

	std::queue<CConnection*>m_connectionQueue;		//�洢mysql���ӵ�����
	std::mutex				m_queueMutex;			//ά�����а�ȫ�Ļ�����

	static CConnectionPool*	m_object;				//��������
	static deobj			heleper;				//��̬��������������
	std::atomic_uint		m_connectionCount;		//��¼������������������
	std::condition_variable m_CV;					//�����������������������̺߳������̵߳�ͨ��
private:
	bool					LoadConfigFile();		//���������ļ�
	void					ProduceConnectionTask();//�����ڶ������߳��У���������µ��߳�
	void					ScannerConnectionTask();//�����ڶ������߳��У�����ɨ�賬ʱ�����ӣ��������ͷ�
};

