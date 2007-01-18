%{
#include "gAIBase.h"
%}

%feature("director") gSimpleAI;

class gSimpleAI
{
public:
    gSimpleAI();
    inline REAL Think();
    virtual ~gSimpleAI();
    gCycle * Object();
    void SetObject( gCycle * cycle );
protected:
    // default implementation: do nothing all day long
    virtual REAL DoThink() = 0;
private:
    // tJUST_CONTROLLED_PTR< gCycle > object_;
};

%feature("director") gSimpleAIFactory;

class gSimpleAIFactory: public tListItem< gSimpleAIFactory >
{
public:
    gSimpleAI * Create( gCycle * object ) const;    
    static gSimpleAIFactory * Get();    
    static void Set( gSimpleAIFactory * factory );
protected:
    virtual gSimpleAI * DoCreate() const = 0;
private:
    static gSimpleAIFactory *factory_;
};
