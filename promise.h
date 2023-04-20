#ifndef PROMISE_H
#define PROMISE_H

#include <QObject>
#include <functional>
#include <QtConcurrent>
#include <type_traits>

template<typename R, typename... A>
R ret(R(*)(A...)) {};

template<typename C, typename R, typename... A>
R ret(R(C::*)(A...)) {};

template <typename T>
// === SForwardValue (Primary) ==========================================================
struct SForwardValue
{
    SForwardValue(std::remove_reference_t<T> _value) : value(std::move(_value)) {}
    T value;
};

template <>
// === SForwardValue (Specialization) ===================================================
struct SForwardValue<void>
{
};

template <typename T>
// == CPromiseBase (Primary) ============================================================
class CPromiseBase
{
public:
    virtual ~CPromiseBase() = default;
    virtual void call(const SForwardValue<T>& res) = 0;
};

template <typename T, typename R, typename F, typename... Args>
// == CPromise (Primary template) =======================================================
class CPromise : public CPromiseBase<T> {};

template <typename T, typename R, typename F, typename... Args>
// == CPromise (Specialization) =========================================================
class CPromise<T, R, F(Args...)> : public CPromiseBase<T>
{
public:

    explicit CPromise(F&& f, Args&&... args) :
        CPromiseBase<T>(),
        m_fn(std::forward<F>(f)),
        m_args(std::forward<Args>(args)...) {}

    ~CPromise() { if (m_promise) { delete m_promise; m_promise = nullptr; } }

public:

    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args)
    {
        if constexpr (!std::is_class_v<Fn>)
        {
            auto pPromise = new CPromise<R, decltype(ret(f)), Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
            m_promise = pPromise;
            return pPromise;
        }
        else if constexpr (std::is_invocable_v<Fn, R>)
        {
            auto pPromise = new CPromise<R, typename std::invoke_result_t<Fn, R>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
            m_promise = pPromise;
            return pPromise;
        }
        else
        {
            auto pPromise = new CPromise<R, typename std::invoke_result_t<Fn>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
            m_promise = pPromise;
            return pPromise;
        }
    }

    void call(const SForwardValue<T>& res) override
    {
        if constexpr (!std::is_class_v<F>) // Invoke class method and simple function
        {
            if constexpr (std::is_void_v<R>)
            {
                std::apply(m_fn, m_args);
                if (m_promise) m_promise->call(SForwardValue<void>());
            }
            else
            {
                if (m_promise)
                    m_promise->call(SForwardValue<R>(std::apply(m_fn, m_args)));
            }
        }
        else if constexpr (std::is_invocable_v<F, T> && !std::is_void_v<T>) // Invoke lambda
        {
            if constexpr (std::is_void_v<R>)
            {
                std::invoke(m_fn, res.value);
                if (m_promise) m_promise->call(SForwardValue<void>());
            }
            else
            {
                if (m_promise)
                    m_promise->call(SForwardValue<R>(std::invoke(m_fn, res.value)));
            }
        }
        else // Invoke lambda (without arg)
        {
            if constexpr (std::is_void_v<R>)
            {
                std::invoke(m_fn);
                if (m_promise) m_promise->call(SForwardValue<void>());
            }
            else
            {
                if (m_promise)
                    m_promise->call(SForwardValue<R>(std::invoke(m_fn)));
            }
        }
    }

private:
    F m_fn;
    std::tuple<Args...> m_args;
    CPromiseBase<R>* m_promise = nullptr;
};

// == CTaskBase =========================================================================
class CTaskBase : public QObject
{
    Q_OBJECT

public:
    virtual ~CTaskBase() {}

signals:
    void finished();
};

template <typename T>
// == CTask =============================================================================
class CTask : public CTaskBase
{
public:

    explicit CTask(const QFuture<T>& future);
    ~CTask()
    {
        delete m_pWatcher;  m_pWatcher  = nullptr;
        delete m_promise;   m_promise   = nullptr;
    }

public:
    CPromiseBase<T>* promise() { return m_promise; }

public:

    // Create callback
    template <typename F, typename... Args>
    auto then(F&& f, Args&&... args)
    {
        if constexpr (!std::is_class_v<F>)
        {
            auto pPromise = new CPromise<T, decltype(ret(f)), F(Args...)>(std::forward<F>(f), std::forward<Args>(args)...);
            m_promise = pPromise;
            return pPromise;
        }
        else if constexpr (std::is_invocable_v<F, T>)
        {
            auto pPromise = new CPromise<T, typename std::invoke_result_t<F, T>, F(Args...)>(std::forward<F>(f), std::forward<Args>(args)...);
            m_promise = pPromise;
            return pPromise;
        }
        else
        {
            auto pPromise = new CPromise<T, typename std::invoke_result_t<F>, F(Args...)>(std::forward<F>(f), std::forward<Args>(args)...);
            m_promise = pPromise;
            return pPromise;
        }
    }

private:

    void callPromise()
    {
        if (m_promise)
        {
            if constexpr (std::is_void_v<T>)
            {
                m_promise->call(SForwardValue<void>());
            }
            else
            {
                m_promise->call(SForwardValue<T>(m_pWatcher->result()));
            }
        }
    }

private:
    QFutureWatcher<T>*  m_pWatcher  = nullptr;
    CPromiseBase<T>*    m_promise   = nullptr;
};

template<typename T>
CTask<T>::CTask(const QFuture<T>& future) : CTaskBase()
{
    m_pWatcher = new QFutureWatcher<T>;

    connect(m_pWatcher, &QFutureWatcher<T>::finished, this, [=] {
        callPromise();
        emit finished();
    });

    m_pWatcher->setFuture(future);
}

#endif // PROMISE_H