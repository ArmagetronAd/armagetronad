%{
#include "tLinkedList.h"
%}

class tListItemBase
{
public:
    // typedef int (Comparator)( const tListItemBase* a, const tListItemBase* b );
    // void Sort( Comparator* comparator );
    tListItemBase();
    tListItemBase(tListItemBase *&a);
    void Remove();
    void Insert(tListItemBase *&a);
    virtual ~tListItemBase();
    tListItemBase *Next();
    int Len();
};

template <class T> class tListItem : public tListItemBase
{
public:
    tListItem():tListItemBase();
    tListItem(T *&a);
    T *Next();
    void Insert(tListItem *&a);
    void Insert(T *&a);
    void Insert(tListItemBase *&a);

    // template< typename comparator >
    // void Sort( );
};
