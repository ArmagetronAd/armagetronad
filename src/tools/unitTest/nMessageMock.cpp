#include "nMessageMock.h"

nMessageMock::nMessageMock():out(in.rdbuf()) {}

nMessageMock::~nMessageMock() {}

nMessageMock& nMessageMock::operator<< (const float &x) { out << x; return *this; }
nMessageMock& nMessageMock::operator<< (const int &x) { out << x; return *this; }

nMessageMock& nMessageMock::operator>> (float &x) { in >> x; return *this; }

nMessageMock& nMessageMock::operator>> (int &x) { in >> x; return *this; }

/*
  nMessageMock& nMessageMock::operator<< (const unsigned short &x);
  nMessageMock& nMessageMock::operator>> (unsigned short &x);
*/


