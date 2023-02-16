#include "qiBase.h"

#include <QDebug>

/*------------- ~CQICounterTask ---------------------------*/
CQICounterTask::~CQICounterTask()
{
    for (CTaskBase* pTask : m_lstTasks)
        delete pTask;
}

/*------------------ wait ---------------------------------*/
bool CQICounterTask::wait()
{
    if (!isRunning()) return false;

    while (isRunning());

    return true;
}

/*------------------ wait ---------------------------------*/
bool CQICounterTask::wait(int task)
{
    if (!isTaskExec(task)) return false;

    while (isTaskExec(task));

    return true;
}

/*------------------ addTask ------------------------------*/
void CQICounterTask::addTask(int task)
{
    QMutexLocker locker(&m_mutex);
    m_lstKeyTasks.append(task);
}

/*------------------ delTask ------------------------------*/
void CQICounterTask::delTask(int task)
{
    QMutexLocker locker(&m_mutex);
    m_lstKeyTasks.removeOne(task);
}

/*------------------ deleteTask ---------------------------*/
void CQICounterTask::deleteTask()
{
    CTaskBase* pTask = qobject_cast<CTaskBase*>(sender());
    if (pTask)
    {
        if (!m_lstTasks.removeOne(pTask))
        {
            qDebug() << "ERROR: Task not deleted!";
        }
        else delete pTask;
    }
}

/*------ CQIBase ------------------------------------------*/
CQIBase::CQIBase(/* Connection* pDbCon */) :
    CQICounterTask()//,
    //m_dbCon(pDbCon)
{

}

/*----------- isReadyExec ---------------------------------*/
bool CQIBase::isReadyExec()
{
    /*if (!m_dbCon) return false;
    if (!m_dbCon->isConnected())
    {
        emit notification("Соединение потеряно");
        return false;
    }*/

    return true;
}

/*----------- redirectSignals -----------------------------*/
void CQIBase::redirectSignals(const QObject* sender)
{
    const QMetaObject* metaSender = sender->metaObject();

    while (metaSender)
    {
        if (strcmp(metaSender->className(), "QObject") == 0) break;

        for (int i = metaSender->methodOffset(); i < metaSender->methodCount(); ++i)
        {
            QMetaMethod method = metaSender->method(i);

            if (method.methodType() != QMetaMethod::Signal) continue;

            QByteArray signature = method.methodSignature();

            if (signalExists(signature.constData()))
            {
                const char* signal = makeSignal(signature);
                connect(sender, signal, this, signal);
            }
            else
                qDebug()
                << "WARNING! Не удалость сделать переадресацию сигнала: "
                << metaSender->className() << "::"
                << signature;
        }

        metaSender = metaSender->superClass();
    }
}

/*------------------ makeSignal ---------------------------*/
const char* CQIBase::makeSignal(QByteArray signature)
{
    signature.insert(0, SIGNAL());
    return signature.constData();
}

/*----------- signalExists --------------------------------*/
bool CQIBase::signalExists(const char* signature)
{
    const QMetaObject* metaOwn = metaObject();

    while (metaOwn)
    {
        if (strcmp(metaOwn->className(), "QObject") == 0) break;

        for (int i = metaOwn->methodOffset(); i < metaOwn->methodCount(); ++i)
        {
            QMetaMethod method = metaOwn->method(i);
            if (method.methodType() != QMetaMethod::Signal) continue;
            if (strcmp(signature, method.methodSignature()) == 0) return true;
        }

        metaOwn = metaOwn->superClass();
    }

    return false;
}

/*----------- registerThread ------------------------------*/
void CQIBase::registerThread()
{
    qDebug() << "Register thread";
    //m_dbCon->registerThread();
}

/*----------- unregisterThread ----------------------------*/
void CQIBase::unregisterThread()
{
    qDebug() << "Unregister thread";
    //m_dbCon->unregisterThread();
}

