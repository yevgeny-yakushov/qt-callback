#include "queryImpl.h"

#include <thread>
#include <QDebug>

/*--------- CQueryImpl ------------------------------------*/
CQueryImpl::CQueryImpl() : CQIBase()
{
}

CQueryImpl::~CQueryImpl()
{
}

int testFn(int val1, int val2)
{
    qDebug() << "TEST FN\t" << val1 << "\t" << val2 << "\t";

    return 1;
}

void CQueryImpl::startTest(int val1, int val2)
{
    startTask(ETASKS_TESTING, &CQueryImpl::largeMethodVoid, this, 200, 50, 100)

        ->then(&CQueryImpl::callBackTest, this, std::move(val1), std::move(val2))
        ->then(&CQueryImpl::largeMethod, this, 100, 50, 300)
        ->then([=](int res) { qDebug() << "THIS IS LAMBDA\t" << res; })
        ->then(&CQueryImpl::largeMethod, this, 300, 70, 100)
        ->then([=](int res) { qDebug() << "THIS IS LAMBDA\t" << res; });

        // Example for non void
        //->then([=](int res)         { qDebug() << "THIS IS LAMBDA\t" << res; })
        //->then([=]() -> int         { qDebug() << "THIS IS LAMBDA\t"; return 5; })
        //->then([=](int res) -> int  { qDebug() << "THIS IS LAMBDA\t" << res; return 5; })

        // Example for void
        //->then([=]() -> int         { qDebug() << "THIS IS LAMBDA\t"; return 5; })
        //->then([=](int res)         { qDebug() << "THIS IS LAMBDA\t" << res + 5; return res;  })
        //->then([=](int res) -> int  { qDebug() << "THIS IS LAMBDA\t" << res; return 5; });

    startTask(ETASKS_TESTING_LAMBDA, [=]() {
        qDebug() << "Thread lambda 1";
    })->then([]() { qDebug() << "After thread 1 lambda: "; });


    startTask(ETASKS_TESTING_LAMBDA, [=]() -> std::vector<double> {
        qDebug() << "Thread lambda 2";

        std::vector<double> vec(100000000, 12345678.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        return vec;
    })->then([](const std::vector<double>& res) -> const std::vector<double>& {
        qDebug() << "Result from thread lambda: " << res.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        return res;
    })->then([](const std::vector<double>& res) {
        qDebug() << "Result from thread 2 lambda: " << res.size();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    });
}

void CQueryImpl::callBackTest(int val1, int val2)
{
    qDebug() << "TEST CALLBACK\t" << val1 << "\t" << val2 << "\t";
}

int CQueryImpl::largeMethod(int val1, int val2, int val3)
{
    long long res = 0;
    for (int i = 0; i < val1; i++)
    {
        res += (i - val2) + val3;
    }

    return res;
}

void CQueryImpl::largeMethodVoid(int val1, int val2, int val3)
{
    long long res = 0;
    for (int i = 0; i < val1; i++)
    {
        res += (i - val2) + val3;
    }
}
