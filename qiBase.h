#ifndef QIBASE_H
#define QIBASE_H

//#include "Kernel/OdsInterface.h"
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
    CQIBase();
    //CQIBase(ODS::OdsInterface* pIFace);
    ~CQIBase() {}

protected:
    //bool                    isReadyExec();
    void                    redirectSignals(const QObject* sender); // См. описание после класса

protected:

    // Call class method and simple function -> return type is void
    template <typename F, typename... Args>
    auto startTask(int task, F&& f, Args&&... args) -> typename std::enable_if<std::is_void<decltype(ret(f))>::value, CTask<void>*>::type
    {
        auto wrapperFn = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        return createTask(task, QtConcurrent::run([=]() {
            addTask(task);
            registerThread();
            wrapperFn();
            unregisterThread();
            delTask(task);
        }));
    }

    // Call class method and simple function -> return type is non void
    template <typename F, typename... Args>
    auto startTask(int task, F&& f, Args&&... args) -> typename std::enable_if<!std::is_void<decltype(ret(f))>::value, CTask<decltype(ret(f))>*>::type
    {
        auto wrapperFn = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        return createTask(task, QtConcurrent::run([=]() -> auto {
            addTask(task);
            registerThread();
            auto res = wrapperFn();
            unregisterThread();
            delTask(task);
            return res;
        }));
    }

    // Call lambda -> return type is void
    template <typename F>
    auto startTask(int task, F&& f) -> typename std::enable_if<std::is_class_v<F> && std::is_void_v<std::result_of_t<F()>>, CTask<void>*>::type
    {
        auto wrapperFn = std::bind(std::forward<F>(f));

        return createTask(task, QtConcurrent::run([=]() {
            addTask(task);
            registerThread();
            wrapperFn();
            unregisterThread();
            delTask(task);
        }));
    }

    // Call lambda -> return type is non void
    template <typename F>
    auto startTask(int task, F&& f) -> typename std::enable_if<std::is_class_v<F> && !std::is_void_v<std::result_of_t<F()>>, CTask<typename std::result_of_t<F()>>*>::type
    {
        auto wrapperFn = std::bind(std::forward<F>(f));

        return createTask(task, QtConcurrent::run([=]() -> auto {
            addTask(task);
            registerThread();
            auto res = wrapperFn();
            unregisterThread();
            delTask(task);
            return res;
        }));
    }

private:
    // Добавляет префикс к сигнатуре, чтобы Qt понял, это что сигнал: someSignal() -> 2someSignal(), = SIGNAL(someSignal())
    const char* makeSignal(QByteArray signature);
    // Проверяет наличии сигнала с сигнатурой в собственном классе
    bool                    signalExists(const char* signature);

private: // Обертка в ODS register и unregister thread
    void                    registerThread();
    void                    unregisterThread();

signals:
    void                    notification(const QString& ntf);
    void                    notifyError(const QString& ntf);

protected:
    //ODS::OdsInterface* m_pIFace;
};

#endif // QIBASE_H
