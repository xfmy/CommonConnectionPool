#pragma once
#include <mysql.h>
#include <string>
#include <ctime>
// ���ݿ������
class CConnection
{
public:
	CConnection();							// ��ʼ�����ݿ�����

	~CConnection();								// �ͷ����ݿ�������Դ

	bool			connect(std::string ip, unsigned short port, std::string user, std::string password,
		std::string dbname);					// �������ݿ�

	bool			update(std::string sql);	// ���²��� insert��delete��update

	MYSQL_RES*		query(std::string sql);		// ��ѯ���� select
	void			refreshAliveTime();			//ˢ��һ�����ӵ���ʼ�Ŀ���ʱ���
	std::clock_t	GetAliceTime();				//���ش���ʱ��
private:
	MYSQL*			_conn;						// ��ʾ��MySQL Server��һ������
	std::clock_t	m_alivetTime;
};