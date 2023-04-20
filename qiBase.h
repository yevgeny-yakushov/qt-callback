#ifndef QIBASE_H
#define QIBASE_H

#include "promise.h"

#include <QMutexLocker>

class CQICounterTask : public QObject
{
    Q_OBJECT

public:
    CQICounterTask() : QObject() {}
    ~CQICounterTask();

public:
    template<typename T>
    CTask<T>* createTask(int keyTask, const QFuture<T>& future);

public:
    bool                isRunning       ()          { QMutexLocker locker(&m_mutex); return m_lstKeyTasks.size() > 0;       }
    bool                isTaskExec      (int task)  { QMutexLocker locker(&m_mutex); return m_lstKeyTasks.contains(task);   }
    int                 countThread     ()          { QMutexLocker locker(&m_mutex); return m_lstKeyTasks.size();           }
    QList<int>          taskList        ()          { QMutexLocker locker(&m_mutex); return m_lstKeyTasks;                  }

public:
    bool                wait();
    bool                wait(int task);

protected:
    void                addTask(int task);
    void                delTask(int task);

private slots:
    void                deleteTask();

private:
    QList<int>          m_lstKeyTasks;
    QList<CTaskBase*>   m_lstTasks;
    mutable QMutex      m_mutex;
};

template<typename T>
CTask<T>* CQICounterTask::createTask(int keyTask, const QFuture<T>& future)
{
    CTask<T>* pTask = new CTask<T>(future);

    m_lstTasks.append(pTask);

    connect(pTask, SIGNAL(finished()), this, SLOT(deleteTask()));

    return pTask;
}

//=======================================================================================

class CQIBase : public CQICounterTask
{
    Q_OBJECT

protected:
    CQIBase(/* Connection* pDbCon */);
    ~CQIBase() {}

protected:
    bool    isReadyExec();
    void    redirectSignals(const QObject* sender);

protected:

    // Call class method, simple function and lambda
    template <typename F, typename... Args>
    auto startTask(int task, F&& f, Args&&... args) -> typename CTask<std::invoke_result_t<F, Args...>>*
    {
        auto wrapperFn = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        using R = std::invoke_result_t<F, Args...>;

        return createTask(task, QtConcurrent::run([=]() -> R {
            addTask(task);
            registerThread();
            if constexpr (std::is_void_v<R>)
            {
                wrapperFn();
                unregisterThread();
                delTask(task);
            }
            else
            {
                R res = wrapperFn();
                unregisterThread();
                delTask(task);
                return res;
            }
        }));
    }

private:
    // ƒобавл€ет префикс к сигнатуре, чтобы Qt пон€л, это что сигнал: someSignal() -> 2someSignal(), = SIGNAL(someSignal())
    const char* makeSignal(QByteArray signature);
    // ѕровер€ет наличии сигнала с сигнатурой в собственном классе
    bool                    signalExists(const char* signature);

private:
    void                    registerThread();
    void                    unregisterThread();

signals:
    void                    notification(const QString& ntf);
    void                    notifyError(const QString& ntf);

protected:
    //Some connection pointer to DB
    // Connection* m_dbCon;
};

#endif // QIBASE_H
