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
    // TODO
    //tCHECKED_PTR(ePlayerNetID)    player; // the player controlling this cycle.
    REAL laggometer;        //!< the actual best estimate for lag
    REAL laggometerSmooth;  //!< the lag, smoothed over time
    virtual void ReceiveControlNet(nMessage &m);
    virtual ~eNetGameObject();
    void SetPlayer(ePlayerNetID* player);
    virtual nMachine & DoGetMachine() const;  //!< returns the machine this object belongs to
public:
    virtual bool ActionOnQuit();
    virtual void ActionOnDelete();    
    virtual void InitAfterCreation();
    eNetGameObject(eGrid *grid, const eCoord &pos,const eCoord &dir,ePlayerNetID* p,bool autodelete=false);
    eNetGameObject(nMessage &m);
    virtual void RemoveFromGame(); // just remove it from the lists and unregister it
    virtual void WriteCreate(nMessage &m);
    virtual void WriteSync(nMessage &m);
    virtual void ReadSync(nMessage &m);
    virtual bool ClearToTransmit(int user) const;
    virtual bool SyncIsNew(nMessage &m);
    virtual void AddRef();          //!< adds a reference
    virtual void Release();         //!< removes a reference
    virtual void SendControl(REAL time,uActionPlayer *Act,REAL x);
    virtual void ReceiveControl(REAL time,uActionPlayer *Act,REAL x);
    virtual bool Timestep(REAL currentTime);
    ePlayerNetID *Player()const{return player;}
    void clientside_action(){lastClientsideAction=lastTime;}
    virtual REAL Lag() const;
    virtual REAL LagThreshold() const;
};
