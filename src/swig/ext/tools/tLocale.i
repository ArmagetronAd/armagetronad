%{
#include "tLocale.h"
%}

%rename(Output) tOutput;
class tOutput{
public:
    tOutput();
    tOutput(const char * identifier);
    ~tOutput();
%rename(add_literal) AddLiteral;
    void AddLiteral(const char *);       // adds a language independent string
%rename(add_locale) AddLocale;
    void AddLocale(const char *);        // adds a language dependant string
%rename(add_space) AddSpace;
    void AddSpace();                     // adds a simple space
    
%rename(set_template_parameter_string) SetTemplateParameter(int, const char *);
    tOutput & SetTemplateParameter(int num, const char *parameter);
%rename(set_template_parameter_float) SetTemplateParameter(int, float);
    tOutput & SetTemplateParameter(int num, float       parameter);
%rename(clear) Clear;
    void Clear();
%rename(append) Append;
    void Append(const tOutput &o);
%rename(is_empty) IsEmpty;
    bool IsEmpty() const;
};

