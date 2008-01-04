#include "nMessageMock.h"

nMessageMock::nMessageMock():out(),in(out.rdbuf()) {}
nMessageMock::~nMessageMock() {}

void nMessageMock::receive()
{
    //  out(in.rdbuf());

    std::cerr << ">>" << out.str() << "<<" << std::endl;
}

void nMessageMock::Write(unsigned short x) { out << x; }
void nMessageMock::Read(unsigned short &x) { in >> x; }

nMessageMock& nMessageMock::operator<< (const float &x) { out << x; return *this; }
nMessageMock& nMessageMock::operator<< (const int &x) { out << x; return *this; }
nMessageMock& nMessageMock::operator<< (const unsigned short &x) { out << x; return *this; }

nMessageMock& nMessageMock::operator>> (float &x) { in >> x; return *this; }
nMessageMock& nMessageMock::operator>> (int &x) { in >> x; return *this; }
nMessageMock& nMessageMock::operator>> (unsigned short &x) { in >> x; return *this; }



