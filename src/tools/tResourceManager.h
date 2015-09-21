#ifndef ArmageTron_RESOURCEMANAGER_H
#define ArmageTron_RESOURCEMANAGER_H

#include "tString.h"

//! resource manager: fetches and caches resources from repositories or arbitrary URIs
class tResourceManager {
public:
    //! Return the position of the resource in the cache
    static tString locateResource(const char *file, const char *uri="");
    //! opens a resource
    static FILE *openResource(const char *pathname, const char *uri="");

    //! server determined resource repository
    static tString & AccessRepoServer();

    //! client determined resource repository
    static tString & AccessRepoClient();
};

#endif //ArmageTron_RESOURCEMANAGER_H
