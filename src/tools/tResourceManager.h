#ifndef ArmageTron_RESOURCEMANAGER_H
#define ArmageTron_RESOURCEMANAGER_H

#include "tString.h"

//! resource manager: fetches and caches resources from repositories or arbitrary URIs
class tResourceManager {
public:
    enum Result
    {
        OK = 0,
        ERROR_UNKNOWN = 1,
        ERROR_URI = 2,      // URI not well formed
        ERROR_NOTFOUND = 3, // URI not found
        ERROR_NOACCESS = 4  // Access denied
    };

    // fetches an URI and stores it in the provided stream, up to maxLen bytes
    static Result FetchURI(const char* URI, std::ostream& o, int maxLen = -1);

    //! Return the position of the resource in the cache
    static tString locateResource(const char *uri, const char *file);
    //! opens a resource
    static FILE *openResource(const char *uri, const char *pathname);

    //! server determined resource repository
    static tString resRepoServer;

    //! client determined resource repository
    static tString resRepoClient;
};

#endif //ArmageTron_RESOURCEMANAGER_H
