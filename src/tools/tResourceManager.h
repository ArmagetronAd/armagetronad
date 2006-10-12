#ifndef ArmageTron_RESOURCEMANAGER_H
#define ArmageTron_RESOURCEMANAGER_H

#include "tString.h"
#include "tResource.h"

typedef int (*tNewResourceFunc)(const char* path);

//! resource manager: fetches and caches resources from repositories or arbitrary URIs
class tResourceManager {
public:
    //! When finished, this will be the preferred way to load a resource.
    /**
            @param file the full path to the file
            @param typeID the unique identifier for the resource type, which is in turn returned
                    from RegisterResourceType
            @see RegisterResourceType
    */
    static tResource* GetResource(const char *file, int typeID);

    //! Register a resource type.
    /**
            @param func a function that returns a new instance of the resource
            @return the unique identifier for the function.  Use this in subsequence calls to GetResource.

            When you create a new resource type, you must register a function that will instantiate
            that new type.  Later on you'll call tResourceManager to instantiate the type for a
            resource that's needed and you'll need to know the identifier returned from this method.
     */
    static int RegisterResourceType(tNewResourceFunc func);

    //! Return the position of the resource in the cache
    static tString locateResource(const char *file, const char *uri="");
    //! opens a resource
    static FILE *openResource(const char *pathname, const char *uri="");

    //! server determined resource repository
    static tString & AccessRepoServer();

    //! client determined resource repository
    static tString & AccessRepoClient();

    //! register a resource component loader
    // todo: finish this
    static void RegisterLoader();
};

#endif //ArmageTron_RESOURCEMANAGER_H
