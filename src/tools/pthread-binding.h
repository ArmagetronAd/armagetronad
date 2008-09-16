#include <pthread.h>

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
