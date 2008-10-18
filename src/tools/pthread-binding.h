#include <deque>
#include <pthread.h>
#include <stdexcept>

class tPThreadMutex
{
private:
    pthread_mutex_t mutex;
public:
    tPThreadMutex() {
        // TODO: error checking
        pthread_mutexattr_t mta;
        
        pthread_mutex_init(&mutex, &mta);
    }
    void acquire() {
        pthread_mutex_lock(&mutex);
    };
    void release() {
        pthread_mutex_unlock(&mutex);
    };
};

class tPThreadRecursiveMutex
 : public tPThreadMutex
{
private:
    pthread_mutex_t mutex;
public:
    tPThreadRecursiveMutex() {
        // TODO: error checking
        pthread_mutexattr_t mta;
        
        pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &mta);
    }
};

template <class T>
class tPThreadGuard
{
private:
    T*mutex;
public:
    tPThreadGuard(T*m) {
        mutex = m;
        mutex->acquire();
    };
    ~tPThreadGuard() {
        mutex->release();
    };
};

template <class T, class MutexT>
class tPThreadQueue
{
    MutexT mutex;
    std::deque<T> q;
public:
    virtual void add(const T& item) {
        tPThreadGuard<MutexT> mL(&mutex);
        
        q.push_back(item);
    }
    
    virtual size_t size() {
        tPThreadGuard<MutexT> mL(&mutex);
        
        return q.size();
    }
    
    virtual T next() {
        tPThreadGuard<MutexT> mL(&mutex);

        if(q.size() == 0)
            // TODO: throw a specific exception?
            throw std::exception();

        T item = q.front();
        q.pop_front();
        
        return item;
    }
};
