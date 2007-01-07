%{
#include "tLocale.h"
%}

class tOutput{
public:
    tOutput();
    ~tOutput();
    void AddLiteral(const char *);       // adds a language independent string
    void AddLocale(const char *);        // adds a language dependant string
    void AddSpace();                     // adds a simple space
    
%rename(set_template_parameter_string) SetTemplateParameter(int, const char *);
%rename(set_template_parameter_float) SetTemplateParameter(int, float);
    tOutput & SetTemplateParameter(int num, const char *parameter);
    tOutput & SetTemplateParameter(int num, float       parameter);
    void Clear();
    tOutput(const char * identifier);
    void Append(const tOutput &o);
    bool IsEmpty() const;
};

