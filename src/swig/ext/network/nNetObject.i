%{
#include "nNetObject.h"
%}

%feature("director") nNetObject;

class nNetObject{
    friend class nWaitForAckSync;
    friend class nProtoBufNetControlDescriptor;

    bool createdLocally;		 // was the object created on this computer? (alternative: it was created on remote order)
    unsigned long int lastSyncID_;  // the extended id of the last accepted sync message

private:
    unsigned short id;  // the global id; this is the same on all
    mutable int refCtr_; // how many references from
    unsigned short owner; // who owns this object?
    // TODO
    //mutable tCONTROLLED_PTR( nObserver ) observer_;  // this object's observer
    int syncListID_;                                 // ID for the list of objects to sync
public:
    class nKnowsAboutInfo{
    public:
    tJUST_CONTROLLED_PTR< nProtoBufMessageBase > lastSync_;
    bool knowsAboutExistence:1; // is the creation message through?
    bool nextSyncAck:1;         // should the next sync message wait
    bool syncReq:1;              // should a sync message be sent?
    unsigned char  acksPending:4;          // how many messages are underway?
    nKnowsAboutInfo();
    void Reset();
    };
protected:
    nKnowsAboutInfo knowsAbout[MAXCLIENTS+2];
    nNetObject *Object(int i);
    void DoBroadcastExistence();
public:
    static bool DoDebugPrint(); // tells ClearToTransmit to print reason of failure
    static nNetObject *ObjectDangerous(int i );
    virtual void AddRef(); // call this every time you add a pointer
    virtual void Release(); // the same if you remove a pointer to this.
    int GetRefcount() const; // get refcount. Use only for Debgging purposes, never base any decisions on it.
    virtual void ReleaseOwnership(); // release the object only if it was created on this machine
    virtual void TakeOwnership(); // treat an object like it was created locally
    bool Owned(){ return createdLocally; } //!< returns whether the object is owned by this machine
    nObserver& GetObserver() const;    // retunrs the observer of this object
    virtual void Dump( tConsole& con ); // dumps object stats
    unsigned short ID() const;
    unsigned short Owner() const;
    inline nMachine & GetMachine() const;  //!< returns the machine this object belongs to
    virtual nDescriptor& CreatorDescriptor() const=0;
    nNetObject(int owner=-1); // sn_netObjects can be normally created on the server
    nNetObject(nMessage &m); // or, if initially created on the
    virtual void InitAfterCreation(); // after remote creation,
    void Register( const nNetObjectRegistrar& r );    // register with the object database
protected:
    virtual ~nNetObject();
    virtual nMachine & DoGetMachine() const;  //!< returns the machine this object belongs to
public:
    virtual bool ActionOnQuit();
    virtual void ActionOnDelete();
    virtual bool BroadcastExistence();
    virtual void PrintName(tString &s) const;
    bool HasBeenTransmitted(int user) const;
    bool syncRequested(int user) const;
    virtual bool ClearToTransmit(int user) const;
    virtual void WriteSync(nMessage &m); // store sync message in m
    virtual void ReadSync(nMessage &m); // guess what
    virtual bool SyncIsNew(nMessage &m); // is the message newer
    virtual void WriteCreate(nMessage &m); // store sync message in m
protected:
    static int SyncedUser();
    nMessage *NewControlMessage();
public:
    virtual void ReceiveControlNet(nMessage &m);
    virtual bool AcceptClientSync() const;
    void GetID();			// get a network ID
    void RequestSync(bool ack=true);  // request a sync
    void RequestSync(int user,bool ack); // only for a single user
    static void SyncAll();
    static void ClearAll();
    static void ClearAllDeleted();
    static void ClearKnows(int user, bool clear);
    static void RelabelOnConnect();
};
