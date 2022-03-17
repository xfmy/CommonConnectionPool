#include <iostream>
#include "ConnectionPool.h"
#include "Connection.h"

int main()
{
    std::thread a1([]() {
        std::clock_t time = clock();
        for (int i = 0; i < 1000; i++) {
            CConnection a;
            a.connect("127.0.0.1", 3306, "root", "123123", "chat");
            a.update("insert into user(name,age,sex) values('xf',19,'male')");
        }
        std::cout << "未使用连接池" << (clock() - time)/1000 << "s\n";
        }
    );
    std::thread a2([]() {
        std::clock_t time = clock();
        CConnectionPool* p = CConnectionPool::GetCConnectionPoolObject();
        for (int i = 0; i < 1000; i++) {
            p->GetConnection()->update("insert into user(name,age,sex) values('xf',19,'male')");
        }
        std::cout << "使用连接池" << (clock() - time) / 1000 << "ms\n";
        }
    );
    a1.join();
    a2.join();
}
