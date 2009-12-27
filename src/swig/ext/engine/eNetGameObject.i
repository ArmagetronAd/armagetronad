%{
#include "eNetGameObject.h"
%}

class eNetGameObject:public eGameObject,public nNetObject{
    friend class ePlayerNetID;

    void MyInitAfterCreation();
protected:
    REAL lastClientsideAction;
    REAL lastAttemptedSyncTime;
    REAL pingOverflow;

//    tCHECKED_PTR(ePlayerNetID)    player; // the player controlling this cycle.
    // NULL means the AI.
    REAL laggometer;        //!< the actual best estimate for lag
    REAL laggometerSmooth;  //!< the lag, smoothed over time

    // used to implement the control functions
    virtual void ReceiveControlNet( Network::NetObjectControl const & control );

    // easier to implement conversion helpers: just extract the relevant sub-protbuf.
    virtual nProtoBuf       * ExtractControl( Network::NetObjectControl       & control );
    virtual nProtoBuf const * ExtractControl( Network::NetObjectControl const & control );

    virtual ~eNetGameObject();

    void SetPlayer(ePlayerNetID* player);

    virtual nMachine & DoGetMachine() const;  //!< returns the machine this object belongs to
public:
    virtual bool ActionOnQuit(){
        //    this->Kill();
        return false;
    }

    virtual void ActionOnDelete(){
        RemoveFromGame();
    }

    virtual void InitAfterCreation();

    eNetGameObject(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID* p,bool autodelete=false);

    virtual void DoRemoveFromGame(); // just remove it from the lists and unregister it

    virtual bool ClearToTransmit(int user) const;

    virtual void AddRef();          //!< adds a reference
    virtual void Release();         //!< removes a reference

    // control functions: (were once part of nNetObject.h)
    virtual void SendControl(REAL time,uActionPlayer *Act,REAL x);
    // is called on the client whenever a control key is pressed. This
    // sends a message to the server, who will call
    virtual void ReceiveControl(REAL time,uActionPlayer *Act,REAL x);
    // on his instance of the nNetObject.

    virtual bool Timestep(REAL currentTime);

    ePlayerNetID *Player()const{return player;}

    void clientside_action(){lastClientsideAction=lastTime;}

    virtual REAL Lag() const;
    virtual REAL LagThreshold() const;

    //! creates a netobject form sync data
    eNetGameObject( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender );
    //! reads sync data, returns false if sync was old or otherwise invalid
    void ReadSync( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender );
    //! writes sync data (and initialization data if flag is set)
    void WriteSync( Engine::NetGameObjectSync & sync, bool init ) const;
    //! returns true if sync message is new (and updates 
    bool SyncIsNew( Engine::NetGameObjectSync const & sync, nSenderInfo const & sender );
};

