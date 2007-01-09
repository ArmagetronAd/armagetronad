%{
#include <sstream>
using namespace std;
%}

using namespace std;

// blah, whatever
class istream : virtual public ios
{
public:
    explicit istream (streambuf * sb);
    virtual ~istream();
};

class istringstream : public istream
{
public:
    istringstream(const string& str, ios_base::openmode mode = ios_base::in);
    virtual ~istringstream();
};
