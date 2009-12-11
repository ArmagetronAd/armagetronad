%{
#include "tConfiguration.h"
%}

//! access levels for admin interfaces; lower numeric values are better
enum tAccessLevel
{
    tAccessLevel_Owner = 0,        // the server owner
    tAccessLevel_Admin = 1,        // one of his admins
    tAccessLevel_Moderator = 2,    // one of the moderators
    tAccessLevel_3 = 3,            // reserved
    tAccessLevel_4 = 4,            // reserved
    tAccessLevel_Armatrator = 5,   // reserved
    tAccessLevel_Referee = 6,      // a referee elected by players
    tAccessLevel_TeamLeader = 7,   // a team leader
    tAccessLevel_TeamMember = 8,   // a team member
    tAccessLevel_9 = 9,            // reserved
    tAccessLevel_10 = 10,          // reserved
    tAccessLevel_11 = 11,          // reserved
    tAccessLevel_Local      = 12,  // user with a local account
    tAccessLevel_13 = 13,          // reserved
    tAccessLevel_14 = 14,          // reserved
    tAccessLevel_Remote = 15,      // user with remote account
    tAccessLevel_DefaultAuthenticated = 15,     // default access level for authenticated users
    tAccessLevel_FallenFromGrace = 16,          // authenticated, but not liked
    tAccessLevel_Shunned = 17,          // authenticated, but disliked
    tAccessLevel_18 = 18,          // reserved
    tAccessLevel_Authenticated = 19,// any authenticated player
    tAccessLevel_Program = 20,     // a regular player
    tAccessLevel_21 = 21,          // reserved
    tAccessLevel_22 = 22,          // reserved
    tAccessLevel_23 = 23,          // reserved
    tAccessLevel_24 = 24,          // reserved
    tAccessLevel_25 = 25,          // reserved
    tAccessLevel_Invalid = 255,    // completely invalid level
    tAccessLevel_Default = 20
};

%rename(ConfItemBase) tConfItemBase;
class tConfItemBase
{
public:
    typedef void callbackFunc(void);
    tConfItemBase(const char *title, callbackFunc *cb=0);
    virtual ~tConfItemBase();
    tAccessLevel GetRequiredLevel() const;
    static void LoadAll(std::istream &s);
    static void LoadLine(std::istream &s);
    static tConfItemBase *FindConfigItem(std::string const &name);
    
    %extend {
        static void LoadString(std::string str)
        {
            std::istringstream stream(str);
            tConfItemBase::LoadAll(stream);
        }
    }

    virtual void ReadVal(std::istream &s)=0;
    virtual void WriteVal(std::ostream &s)=0;
};

%rename(AccessLevelSetter) tAccessLevelSetter;
class tAccessLevelSetter
{
public:
    //! modifies the access level of <item> to <level>
    tAccessLevelSetter( tConfItemBase & item, tAccessLevel level );
};

%rename(ConfItemScript) tConfItemScript;
class tConfItemScript:public tConfItemBase{
public:
    tConfItemScript(const char *title, tScripting::proc_type proc);
    virtual ~tConfItemScript();

    virtual void ReadVal(std::istream &s);
    virtual void WriteVal(std::ostream &s);
};
