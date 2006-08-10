#include "values/vCore.h"

namespace vValue {

class Parser {
public:
    static vValue::Expr::Base *parse(tString s);
};

}
