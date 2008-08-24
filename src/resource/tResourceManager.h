#ifndef ArmageTron_RESOURCEMANAGER_H
#define ArmageTron_RESOURCEMANAGER_H

#include <boost/smart_ptr.hpp>

#include "tString.h"

class tResource;

typedef int (*tNewResourceFunc)(const char* path);

//! Shortcut for casting to tResource*
#define resource_cast(a) dynamic_cast<tResource *> (a)

//! resource manager: fetches and caches resources from repositories or arbitrary URIs
/** 
 *   put detailed docs here
 */
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

    //! Convenient typedef for a type that stores references to the resource
    /// manager.
    typedef boost::shared_ptr<tResourceManager> Reference;

    //! tResourceManager is a singleton, and this is how you get the
    /// global instance of it.
    Reference GetResourceManager();

    ~tResourceManager();
private:
    //! The only instance of tResourceManager allowed
    static Reference __inst;

    //! We make the constructor private so that nobody else can
    /// instantiate this class
    tResourceManager();
};

//! helper class to construct a resource path
class tResourcePath
{
    tString m_Author;   //!< the author of the resource
    tString m_Category; //!< the category of the resource
    tString m_Name;     //!< the name of the resource
    tString m_Version;  //!< the version of the resource
    tString m_Type;     //!< the type of the resource
    tString m_Extension;//!< the extension (like xml or png)
    tString m_URI;      //!< the URI to the file, if any
    tString m_Path;     //!< the full path of the resource

    bool m_Valid;
public:
    tString const &Author   () const {return m_Author   ;} //!< get the author of the resource
    tString const &Category () const {return m_Category ;} //!< get the category of the resource
    tString const &Name     () const {return m_Name     ;} //!< get the name of the resource
    tString const &Version  () const {return m_Version  ;} //!< get the version of the resource
    tString const &Type     () const {return m_Type     ;} //!< get the type of the resource
    tString const &Extension() const {return m_Extension;} //!< get the extension (like xml or png)
    tString const &URI      () const {return m_URI      ;} //!< get the URI to the file, if any
    tString const &Path     () const {return m_Path     ;} //!< get the full path of the resource

    bool Valid() const {return m_Valid;} //!< is this a valid resource path?

    //! default constructor. Valid() will return false if called on an object constructed this way.
    tResourcePath():m_Valid(false){}

    //! construct the path from the given arguments
    tResourcePath(tString const &Author,
                  tString const &Category,
                  tString const &Name,
                  tString const &Version,
                  tString const &Type,
                  tString const &Extension,
                  tString const &URI);

    //! construct a path from a resource location
    tResourcePath(tString const &path);

    bool operator==(tResourcePath const &other) const;
    bool operator!=(tResourcePath const &other) const;
};

#endif //ArmageTron_RESOURCEMANAGER_H
