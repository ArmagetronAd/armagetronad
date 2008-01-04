#include <iostream>
#include <sstream>
#include <ostream>

#ifndef N_MESSAGE_MOCK
#define N_MESSAGE_MOCK

class nMessageMock {
protected:
    std::ostringstream out;
    std::istream in;

public:
    nMessageMock();
    ~nMessageMock();

    void receive();
    void Write(unsigned short x);
    void Read(unsigned short &x);

    nMessageMock& operator<< (const float &x);
    nMessageMock& operator>> (float &x);

    nMessageMock& operator<< (const unsigned short &x);
    nMessageMock& operator>> (unsigned short &x);

    nMessageMock& operator<< (const int &x);
    nMessageMock& operator>> (int &x);
};

#endif
