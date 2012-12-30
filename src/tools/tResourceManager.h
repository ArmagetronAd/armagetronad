#ifndef ArmageTron_RESOURCEMANAGER_H
#define ArmageTron_RESOURCEMANAGER_H

#include "tString.h"
#include "tConfiguration.h"

//! resource manager: fetches and caches resources from repositories or arbitrary URIs
class tResourceManager {
public:
    //! Return the position of the resource in the cache
    static tString locateResource(const char *uri, const char *file);
    //! opens a resource
    static FILE *openResource(const char *uri, const char *pathname);

    //! server determined resource repository
    static tString resRepoServer;

    //! client determined resource repository
    static tString resRepoClient;
};

class tSettingResource: public tConfItemBase
{
public:
    tSettingResource( char const * name )
    : tConfItemBase( name ),
      current_(0)
    {
    }

    // the number of items
    int Size() const
    {
        return items_.Len();
    }

    // returns the current value
    tString Current() const
    {
        tASSERT( Size() > 0 && current_ >= 0 && current_ < Size() );

        return items_[current_];
    }

void Reset()
    {
        current_ = 0;
    }

    tString Get(int itemID) const
    {
        return items_[itemID];
    }
private:
    virtual void ReadVal( std::istream &is )
    {
        tString mapsT;
        mapsT.ReadLine (is);
        items_.SetLen (0);

        int strpos = 0;
        int nextsemicolon = mapsT.StrPos(";");

        if (nextsemicolon != -1)
        {
            do
            {
                tString const &map = mapsT.SubStr(strpos, nextsemicolon - strpos);

                strpos = nextsemicolon + 1;
                nextsemicolon = mapsT.StrPos(strpos, ";");

                items_.Insert(map);
            }
            while ((nextsemicolon = mapsT.StrPos(strpos, ";")) != -1);
        }

        // make sure the current value is correct
        if ( current_ >= items_.Len() )
        {
            current_ = 0;
        }
    }

    virtual void WriteVal(std::ostream &s){}
    virtual bool Writable(){return false;}
    virtual bool Save(){return false;}

    tArray<tString> items_; // the list of resource repository hosts
    int current_;           // the index of the current
};

#endif //ArmageTron_RESOURCEMANAGER_H
