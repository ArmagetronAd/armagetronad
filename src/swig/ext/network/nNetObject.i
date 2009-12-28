%{
#include "nNetObject.h"
%}

%feature("director") nNetObject;

class nNetObject{
public:
    static nNetObject *Object(int i);
    // returns a pointer to the nNetObject
    // with id=i. If that does not exist yet, wait for it to spawn,
    // or, on the server, kill the person responsible.
    // should be only called in constructors.

protected:

    nKnowsAboutInfo knowsAbout[MAXCLIENTS+2];

    void DoBroadcastExistence();
public:
    static bool DoDebugPrint(); // tells ClearToTransmit to print reason of failure

    static nNetObject *ObjectDangerous(int i );
    
    // clears an eventually deleted object of the given ID out of the main lists
    static void ClearDeleted( unsigned short ID );

    virtual void AddRef(); // call this every time you add a pointer
    // to this nNetObject from another nNetObject, so we know when it is
    // safe to delete this.
    virtual void Release(); // the same if you remove a pointer to this.
    // AND: it should be called instead of delete.
    int GetRefcount() const; // get refcount. Use only for Debgging purposes, never base any decisions on it.

    virtual void ReleaseOwnership(); // release the object only if it was created on this machine
    virtual void TakeOwnership(); // treat an object like it was created locally
    bool Owned();

    nObserver& GetObserver() const;		// retunrs the observer of this object
    void ClearCache() const; 			//!< clears the message cache, call on object changes
    virtual void Dump( tConsole& con ); 	// dumps object stats
    unsigned short ID() const;
    unsigned short Owner() const;
    inline nMachine & GetMachine() const;	//!< returns the machine this object belongs to
    nNetObject(int owner=-1); 			// sn_netObjects can be normally created on the server
    virtual void InitAfterCreation(); 		// after remote creation,
    void Register(const nNetObjectRegistrar& r);// register with the object database
protected:
    virtual ~nNetObject();
    virtual nMachine & DoGetMachine() const;	//!< returns the machine this object belongs to
public:
    virtual bool ActionOnQuit();
    virtual void ActionOnDelete();
    virtual bool BroadcastExistence();
    virtual void PrintName(tString &s) const;

    bool HasBeenTransmitted(int user) const;
    bool syncRequested(int user) const;
    virtual bool ClearToTransmit(int user) const;

    //! returns a creation message for this object
    nProtoBufMessageBase * CreationMessage();
    //! returns a sync message for this object
    nProtoBufMessageBase * SyncMessage();

    //! creates a netobject form sync data
    nNetObject( Network::NetObjectSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Network::NetObjectSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Network::NetObjectSync & sync, bool init ) const;
    //! returns true if sync message is new (and updates 
    bool SyncIsNew( Network::NetObjectSync const & sync, nSenderInfo const & sender );

    //! returns the descriptor responsible for this class
    inline nNetObjectDescriptorBase const & GetDescriptor() const { return DoGetDescriptor(); }
private:
    //! returns the descriptor responsible for this class
    virtual nNetObjectDescriptorBase const & DoGetDescriptor() const = 0;

protected:
    Network::NetObjectControl & BroadcastControl();
    // creates a new control message that can be used to control other
    // copies of this nNetObject; control is received with ReceiveControlNet().
    // It is automatically broadcast (sent to the server in client mode).

public:
    virtual void ReceiveControlNet( Network::NetObjectControl const & control );
    // receives the control message. the data written to the message created
    // by *NewControlMessage() can be read directly from m.

    // shall the server accept sync messages from the clients?
    virtual bool AcceptClientSync() const;

    void GetID();			// get a network ID
    void RequestSync(bool ack=true);  // request a sync
    void RequestSync(int user,bool ack); // only for a single user

    // global functions:
    static void SyncAll();
    static void ClearAll();
    static void ClearAllDeleted();
    static void ClearKnows(int user, bool clear);
    static void RelabelOnConnect();
};
