#ifndef QUERYIMPL_H
#define QUERYIMPL_H

#include "qiBase.h"

enum ETASKS
{
    ETASKS_TESTING = 0,
    ETASKS_TESTING_LAMBDA
};

class CQueryImpl : public CQIBase
{
    Q_OBJECT

public:
    explicit CQueryImpl();
            ~CQueryImpl();

public:
    void    startTest       (int val1, int val2);
                                               
public:                                        
    void    callBackTest    (int val1, int val2);

private:
    int     largeMethod     (int val1, int val2, int val3);
    void    largeMethodVoid (int val1, int val2, int val3);
};

#endif // QUERYIMPL_H
