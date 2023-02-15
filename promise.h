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
// == CPromiseBase (Primary) ============================================================
class CPromiseBase
{
public:
    virtual ~CPromiseBase() {}
    virtual void call(T& res) = 0;
};

template <>
// == CPromiseBase (Specialization) =====================================================
class CPromiseBase<void>
{
public:
    virtual ~CPromiseBase() {}
    virtual void call() = 0;
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

    // Lambda with arg
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<std::is_class_v<Fn> && std::is_invocable_v<Fn, R>, CPromise<R, typename std::result_of_t<Fn(R)>, Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda with arg, return type: " << typeid(typename std::result_of_t<Fn(R)>).name();
#endif // DEBUG

        CPromise<R, typename std::result_of_t<Fn(R)>, Fn(FnArgs...)>* pPromise = new CPromise<R, typename std::result_of_t<Fn(R)>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Lambda without arg
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<std::is_class_v<Fn> && !std::is_invocable_v<Fn, R>, CPromise<R, typename std::result_of_t<Fn()>, Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda without arg, return type: " << typeid(typename std::result_of_t<Fn()>).name();
#endif // DEBUG

        CPromise<R, typename std::result_of_t<Fn()>, Fn(FnArgs...)>* pPromise = new CPromise<R, typename std::result_of_t<Fn()>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Class method and simple function
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<!std::is_class_v<Fn>, CPromise<R, decltype(ret(f)), Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Class method and simple function, return type: " << typeid(decltype(ret(f))).name();
#endif // DEBUG

        CPromise<R, decltype(ret(f)), Fn(FnArgs...)>* pPromise = new CPromise<R, decltype(ret(f)), Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    void call(T& res) override { privateCall(res); }

private:

    // Invoke lambda (with arg)
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<std::is_class_v<Q> && std::is_invocable_v<Q, T> && std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, R] 1. Invoke lambda (with arg)";
#endif // DEBUG

        std::apply(m_fn, std::make_tuple(res));

        if (m_promise)
        {
            m_promise->call();
        }
    }

    // Invoke lambda (without arg)
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<std::is_class_v<Q> && std::is_invocable_v<Q, T> && !std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, R] 2. Invoke lambda (with arg)";
#endif // DEBUG

        R lambdaRes = std::apply(m_fn, std::make_tuple(res));

        if (m_promise)
        {
            m_promise->call(lambdaRes);
        }
    }

    // Invoke lambda (without arg) -> return type is void
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<std::is_class_v<Q> && !std::is_invocable_v<Q, T> && std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, R] 3. Invoke lambda (without arg)";
#endif // DEBUG

        std::invoke(m_fn);

        if (m_promise)
        {
            m_promise->call();
        }
    }

    // Invoke lambda (without arg) -> return type is non void
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<std::is_class_v<Q> && !std::is_invocable_v<Q, T> && !std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, R] 4. Invoke lambda (without arg)";
#endif // DEBUG

        R lambdaRes = std::invoke(m_fn);

        if (m_promise)
        {
            m_promise->call(lambdaRes);
        }
    }

    // Invoke class method and simple function -> return type is non void
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<!std::is_class_v<Q> && !std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, R] 5. Invoke class method and simple function";
#endif // DEBUG

        R fnRes = std::apply(m_fn, m_args);

        if (m_promise)
        {
            m_promise->call(fnRes);
        }
    }

    // Invoke class method and simple function -> return type is void
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<!std::is_class_v<Q> && std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, R] 6. Invoke class method and simple function";
#endif // DEBUG

        std::apply(m_fn, m_args);

        if (m_promise)
        {
            m_promise->call();
        }
    }

private:
    F m_fn;
    std::tuple<Args...> m_args;
    CPromiseBase<R>* m_promise = nullptr;
};

template <typename R, typename F, typename... Args>
// == CPromise (Specialization) =========================================================
class CPromise<void, R, F(Args...)> : public CPromiseBase<void>
{
public:

    explicit CPromise(F&& f, Args&&... args) :
        CPromiseBase<void>(),
        m_fn(std::forward<F>(f)),
        m_args(std::forward<Args>(args)...) {}

    ~CPromise() { if (m_promise) { delete m_promise; m_promise = nullptr; } }

public:

    // Lambda without arg
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<std::is_class_v<Fn> && std::is_invocable_v<Fn, R>, CPromise<R, typename std::result_of_t<Fn(R)>, Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda without arg, return type: " << typeid(typename std::result_of_t<Fn(R)>).name();
#endif // DEBUG

        CPromise<R, typename std::result_of_t<Fn(R)>, Fn(FnArgs...)>* pPromise = new CPromise<R, typename std::result_of_t<Fn(R)>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Lambda without arg
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<std::is_class_v<Fn> && !std::is_invocable_v<Fn, R>, CPromise<R, typename std::result_of_t<Fn()>, Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda without arg, return type: " << typeid(typename std::result_of_t<Fn()>).name();
#endif // DEBUG

        CPromise<R, typename std::result_of_t<Fn()>, Fn(FnArgs...)>* pPromise = new CPromise<R, typename std::result_of_t<Fn()>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Class method and simple function
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<!std::is_class_v<Fn>, CPromise<R, decltype(ret(f)), Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Class method and simple function, return type: " << typeid(decltype(ret(f))).name();
#endif // DEBUG

        CPromise<R, decltype(ret(f)), Fn(FnArgs...)>* pPromise = new CPromise<R, decltype(ret(f)), Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    void call() override { privateCall(); }

private:

    // Invoke lambda (without arg) -> return type is void
    template <typename Q = F>
    auto privateCall() -> typename std::enable_if<std::is_class_v<Q> && std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[void, R] 1. Invoke lambda (without arg)";
#endif // DEBUG

        std::invoke(m_fn);

        if (m_promise)
        {
            m_promise->call();
        }
    }

    // Invoke lambda (without arg) -> return type is non void
    template <typename Q = F>
    auto privateCall() -> typename std::enable_if<std::is_class_v<Q> && !std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[void, R] 2. Invoke lambda (without arg)";
#endif // DEBUG

        R res = std::invoke(m_fn);

        if (m_promise)
        {
            m_promise->call(res);
        }
    }

    // Invoke class method and simple function -> return type is non void
    template <typename Q = F>
    auto privateCall() -> typename std::enable_if<!std::is_class_v<Q> && !std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[void, R] 3. Invoke class method and simple function";
#endif // DEBUG

        R fnRes = std::apply(m_fn, m_args);

        if (m_promise)
        {
            m_promise->call(fnRes);
        }
    }

    // Invoke class method and simple function -> return type is void
    template <typename Q = F>
    auto privateCall() -> typename std::enable_if<!std::is_class_v<Q> && std::is_void_v<R>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[void, R] 4. Invoke class method and simple function";
#endif // DEBUG

        std::apply(m_fn, m_args);

        if (m_promise)
        {
            m_promise->call();
        }
    }

private:
    F m_fn;
    std::tuple<Args...> m_args;
    CPromiseBase<R>* m_promise = nullptr;
};

template <typename T, typename F, typename... Args>
// == CPromise (Specialization) =========================================================
class CPromise<T, void, F(Args...)> : public CPromiseBase<T>
{
public:

    explicit CPromise(F&& f, Args&&... args) :
        CPromiseBase<T>(),
        m_fn(std::forward<F>(f)),
        m_args(std::forward<Args>(args)...) {}

    ~CPromise() { if (m_promise) { delete m_promise; m_promise = nullptr; } }

public:

    // Lambda with arg
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<std::is_class_v<Fn> && std::is_invocable_v<Fn>, CPromise<void, typename std::result_of_t<Fn()>, Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda with arg, return type: " << typeid(typename std::result_of_t<Fn()>).name();
#endif // DEBUG

        CPromise<void, typename std::result_of_t<Fn()>, Fn(FnArgs...)>* pPromise = new CPromise<void, typename std::result_of_t<Fn()>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Class method and simple function
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<!std::is_class_v<Fn>, CPromise<void, decltype(ret(f)), Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Class method and simple function, return type: " << typeid(decltype(ret(f))).name();
#endif //DEBUG

        CPromise<void, decltype(ret(f)), Fn(FnArgs...)>* pPromise = new CPromise<void, decltype(ret(f)), Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    void call(T& res) override { privateCall(res); }

private:
    // Invoke lambda (with arg)
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<std::is_class_v<Q>&& std::is_invocable_v<Q, T>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, void] 1. Invoke lambda (with arg)";
#endif //DEBUG

        std::apply(m_fn, std::make_tuple(res));

        if (m_promise)
        {
            m_promise->call();
        }
    }

    // Invoke lambda (without arg)
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<std::is_class_v<Q> && !std::is_invocable_v<Q, T>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, void] 2. Invoke lambda (without arg)";
#endif //DEBUG

        std::invoke(m_fn);
        
        if (m_promise)
        {
            m_promise->call();
        }
    }

    // Invoke class method and simple function
    template <typename Q = F>
    auto privateCall(T& res) -> typename std::enable_if<!std::is_class_v<Q>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[T, void] 3. Invoke class method and simple function";
#endif // DEBUG

        std::apply(m_fn, m_args);

        if (m_promise)
        {
            m_promise->call();
        }
    }

private:
    F m_fn;
    std::tuple<Args...> m_args;
    CPromiseBase<void>* m_promise = nullptr;
};

template <typename F, typename... Args>
// == CPromise (Specialization) =========================================================
class CPromise<void, void, F(Args...)> : public CPromiseBase<void>
{
public:

    explicit CPromise(F&& f, Args&&... args) :
        CPromiseBase<void>(),
        m_fn(std::forward<F>(f)),
        m_args(std::forward<Args>(args)...) {}

    ~CPromise() { if (m_promise) { delete m_promise; m_promise = nullptr; } }

public:

    // Lambda without arg
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<std::is_class_v<Fn> && std::is_invocable_v<Fn>, CPromise<void, typename std::result_of_t<Fn()>, Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda without arg, return type: " << typeid(typename std::result_of_t<Fn()>).name();
#endif // DEBUG

        CPromise<void, typename std::result_of_t<Fn()>, Fn(FnArgs...)>* pPromise = new CPromise<void, typename std::result_of_t<Fn()>, Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Class method and simple function
    template <typename Fn, typename... FnArgs>
    auto then(Fn&& f, FnArgs&&... args) -> typename std::enable_if<!std::is_class_v<Fn>, CPromise<void, decltype(ret(f)), Fn(FnArgs...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Class method and simple function, return type: " << typeid(decltype(ret(f))).name();
#endif // DEBUG

        CPromise<void, decltype(ret(f)), Fn(FnArgs...)>* pPromise = new CPromise<void, decltype(ret(f)), Fn(FnArgs...)>(std::forward<Fn>(f), std::forward<FnArgs>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    void call() override { privateCall(); }

private:

    // Invoke lambda (without arg) -> return type is void
    template <typename Q = F>
    auto privateCall() -> typename std::enable_if<std::is_class_v<Q>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[void, void] 1. Invoke lambda (without arg)";
#endif // DEBUG

        std::invoke(m_fn);

        if (m_promise)
        {
            m_promise->call();
        }
    }

    // Invoke class method and simple function
    template <typename Q = F>
    auto privateCall() -> typename std::enable_if<!std::is_class_v<Q>, void>::type
    {
#ifdef DEBUG
        qDebug() << "[void, void] 2. Invoke class method and simple function";
#endif // DEBUG

        std::apply(m_fn, m_args);

        if (m_promise)
        {
            m_promise->call();
        }
    }

private:
    F m_fn;
    std::tuple<Args...> m_args;
    CPromiseBase<void>* m_promise = nullptr;
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
    // Lambda with arg
    template <typename F, typename... Args>
    auto then(F&& f, Args&&... args) -> typename std::enable_if<std::is_class_v<F> && std::is_invocable_v<F, T>, CPromise<T, typename std::result_of_t<F(T)>, F(Args...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda with arg, return type: " << typeid(typename std::result_of_t<F(T)>).name();
#endif // DEBUG

        CPromise<T, typename std::result_of_t<F(T)>, F(Args...)>* pPromise = new CPromise<T, typename std::result_of_t<F(T)>, F(Args...)>(std::forward<F>(f), std::forward<Args>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Lambda without arg
    template <typename F, typename... Args>
    auto then(F&& f, Args&&... args) -> typename std::enable_if<std::is_class_v<F> && !std::is_invocable_v<F, T>, CPromise<T, typename std::result_of_t<F()>, F(Args...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Lambda without arg, return type: " << typeid(typename std::result_of_t<F()>).name();
#endif // DEBUG

        CPromise<T, typename std::result_of_t<F()>, F(Args...)>* pPromise = new CPromise<T, typename std::result_of_t<F()>, F(Args...)>(std::forward<F>(f), std::forward<Args>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

    // Class method and simple function
    template <typename F, typename... Args>
    auto then(F&& f, Args&&... args) -> typename std::enable_if<!std::is_class_v<F>, CPromise<T, decltype(ret(f)), F(Args...)>*>::type
    {
#ifdef DEBUG
        qDebug() << "Class method and simple function, return type: " << typeid(decltype(ret(f))).name();
#endif // DEBUG

        CPromise<T, decltype(ret(f)), F(Args...)>* pPromise = new CPromise<T, decltype(ret(f)), F(Args...)>(std::forward<F>(f), std::forward<Args>(args)...);
        m_promise = pPromise;
        return pPromise;
    }

private:

    template<typename Q = T>
    auto callPromise() -> typename std::enable_if<std::is_void_v<Q>, void>::type
    {
        if (m_promise)
        {
            m_promise->call();
        }
    }

    template<typename Q = T>
    auto callPromise() -> typename std::enable_if<!std::is_void_v<Q>, void>::type
    {
        if (m_promise)
        {
            T res = m_pWatcher->result();
            m_promise->call(res);
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
