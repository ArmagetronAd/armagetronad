#ifndef ArmageTron_RESOURCEMANAGER_H
#define ArmageTron_RESOURCEMANAGER_H

#include "tString.h"

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

extern void RInclude(tString file);

#endif //ArmageTron_RESOURCEMANAGER_H
