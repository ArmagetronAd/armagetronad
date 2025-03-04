#ifndef ArmageTron_RESOURCEMANAGER_H
#define ArmageTron_RESOURCEMANAGER_H

#include "tString.h"

//! resource manager: fetches and caches resources from repositories or arbitrary URIs
class tResourceManager {
public:
    enum Result
    {
        RESULT_Ok = 200,       // all fine
        ERROR_Unknown = -1,    // unknown error
        ERROR_Uri = -2,        // URI not well formed
        ERROR_FileAccess = -3, // target file not writable
        ERROR_NotFound = 404,  // URI not found
        ERROR_NoAccess = 401   // Access denied
    };

    // fetches an URI and stores it in the provided stream
    static Result FetchURI(const char* URI, std::ostream& o);

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
