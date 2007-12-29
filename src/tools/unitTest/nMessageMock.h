#include <iostream>
#include <sstream>
#include <ostream>

#ifndef N_MESSAGE_MOCK
#define N_MESSAGE_MOCK

class nMessageMock {
protected:
  std::istringstream in;
  //  std::istringstream in(ios_base::in| std::ios_base::out);
  std::ostream out;
  
public:
  nMessageMock();
  ~nMessageMock();

  nMessageMock& operator<< (const float &x);
  nMessageMock& operator>> (float &x);

  /*
  nMessageMock& operator<< (const unsigned short &x);
  nMessageMock& operator>> (unsigned short &x);
  */

  nMessageMock& operator<< (const int &x);
  nMessageMock& operator>> (int &x);
};

#endif
