# Потоковый "менеджер" и переадресация сигналов на qt/c++

Класс qiBase позволяет создавать оболочку для обработки методов/функций/лямбд в потоке, а также позволяет обработать результат выполнения с помощью метода then. Метод *::then позволяет реализовать стек callback'ов:

```cpp
    startTask(ETASKS_TESTING, &CQueryImpl::largeMethodVoid, this, 200, 50, 100)
        ->then(&CQueryImpl::callBackTest, this, std::move(val1), std::move(val2))
        ->then(&CQueryImpl::largeMethod, this, 100, 50, 300)
        ->then([=](int res) { qDebug() << "THIS IS LAMBDA\t" << res; })
        ->then(&CQueryImpl::largeMethod, this, 300, 70, 100)
        ->then([=](int res) { qDebug() << "THIS IS LAMBDA\t" << res; });
```

Результат вычисления метода then можно передать дальше по стеку в следующий then метод.

Также реализован потоковый счетчик, который позволяет:
- Узнать, запущены ли какие-нибудь задачи (isRunning())
- Узнать, запущена ли задача (isTaskExec(int task))
- Количество потоков (countThread())
- Получить лист запущенных задач (taskList())
- Дождаться выполнения всех задач (wait())

# Переадресация
Класс qiBase позволяет осущесвтить переадресацию сигналов объектов-мемборов в класс владельца

```cpp
// Метод redirectSignals может перенаправить все сигналы объекта, а также которые наследованы по иерархии

//    Пример:

class A : public QObject
{
    Q_OBJECT
//...
    signals:
        void sig1();
};

class B : public A
{
    Q_OBJECT
//...
    signals:
        void sig2();
};

class C : public CQIBase
{
    Q_OBJECT
//...
    signals:
        void sig1(); // Сигнал объекта B, наследованный от A
        void sig2(); // Сигнал объекта B

        void sig3(); // Собственный сигнал

    private:
        B* m_pB;
};

// По сути, заменяем данную ситуацию:

connect(B, &B::sig1, this, &C::sig1); // коннект в классе С
connect(B, &B::sig2, this, &C::sig2); // коннект в классе С

// на простой вызов метода:

redirectSignals(m_pB);

//    В данном примере всего лишь 2 сигнала, а не 100, например.

//    Главное, чтобы были описаны все необходимые сигналы в классе C с той же сигнатурой, что и в классе B
//    Также есть возможность частичного перенаправления, просто объявить только нужные сигналы,
//    допустим только: void sig1();
//    Тогда будет сделано перенаправление только сигнала sig1, а на sig2, будет выдано предупреждение,
//    что не получилось перенаправить - это не ошибка.
```


