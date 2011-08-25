/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#ifndef ArmageTron_SAFEPTR_H
#define ArmageTron_SAFEPTR_H

#include "aa_config.h"
#include <stddef.h>
#if HAVE_UNISTD_H
// #include <unistd.h>
#endif

#include "tError.h"

class tCheckedPTRBase{
    friend class tPTRList;
    int id;
protected:
    void *target;
public:
    tCheckedPTRBase(void *x);
    tCheckedPTRBase(const tCheckedPTRBase &x);
    tCheckedPTRBase();
    ~tCheckedPTRBase();

    tCheckedPTRBase& operator=(void *x){target=x; return *this;}
    //  void * operator->() const {return target;}
    //  void & operator*() const {return *target;}
    //operator void *() const {return target;}
    operator bool() const{return target!=NULL;}
    bool operator !() const{return !target;}

    static void CheckDestructed(void *test);

    static void Check();
};


template<class T> class tCheckedPTR:public tCheckedPTRBase{
    typedef T myclass;
public:
    tCheckedPTR(T *x):tCheckedPTRBase(x){}
    tCheckedPTR(const tCheckedPTR<T> &x):tCheckedPTRBase(x.target){}
    tCheckedPTR():tCheckedPTRBase(){}
    ~tCheckedPTR(){}

    tCheckedPTR<T> &operator=(T *x){tCheckedPTRBase::operator=(x); return *this;}
    tCheckedPTR<T> &operator=(const tCheckedPTR<T> &x)
    {tCheckedPTRBase::operator=(x.target); return *this;}
    T * operator->() const {return reinterpret_cast<T*>(target);}
    T & operator*() const {return *reinterpret_cast<T*>(target);}
    operator T*() const {return reinterpret_cast<T*>(target);}
    //  T** operator&() {return reinterpret_cast<T **>(&target);}

    bool operator==(const T* x)const{return target==x;}
    bool operator!=(const T* x)const{return target!=x;}
    bool operator==(const tCheckedPTR<T> &x)const{return target==x.target;}
    bool operator!=(const tCheckedPTR<T> &x)const{return target!=x.target;}

    void Destroy(){
        if (target){
            T *dummy=reinterpret_cast<T*>(target);
            target=NULL;
            delete dummy;
        }
    }
};

template<class T> class tCheckedPTRConst:public tCheckedPTRBase{
    typedef T myclass;
public:
    tCheckedPTRConst():tCheckedPTRBase(NULL){}
    tCheckedPTRConst(const T *x):tCheckedPTRBase(reinterpret_cast<void *>(x)){}
    tCheckedPTRConst(const tCheckedPTRConst<T> &x):tCheckedPTRBase(x.target){}
    tCheckedPTRConst(const tCheckedPTR<T> &x):tCheckedPTRBase(x.operator->()){}
    ~tCheckedPTRConst(){}

    tCheckedPTRConst<T> &operator=(const T *x)
    {tCheckedPTRBase::operator=(reinterpret_cast<T *>(x)); return *this;}
    tCheckedPTRConst<T> &operator=(const tCheckedPTRConst<T> &x)
    {tCheckedPTRBase::operator=(x.target); return *this;}
    tCheckedPTRConst<T> &operator=(const tCheckedPTR<T> &x)
    {tCheckedPTRBase::operator=(x.operator->()); return *this;}

    const T * operator->() const {return target;}
    const T & operator*() const {return *reinterpret_cast<T*>(target);}
    operator const T*() const {return reinterpret_cast<const T*>(target);}
    //  const T** operator&() {return reinterpret_cast<const T **>(&target);}

    bool operator==(const T* x)const{return target==x;}
    bool operator!=(const T* x)const{return target!=x;}

    void Destroy(){
        if (target){
            T *dummy=reinterpret_cast<T *>(target);
            target=NULL;
            dummy->AddRef();
            dummy->Release();
        }
    }
};




//#define CHECK_PTR 1


template<class T> void SafeDelete(T &x){
    x.Destroy();
}

template<class T> void SafeDeletePtr(T * &x){
    if (x){
        T *dummy=x;
        x=NULL;
        delete dummy;
    }
}

#define tDESTROY_PTR(x) SafeDeletePtr(x)

#ifdef DEBUG
#ifdef CHECK_PTR 
#define tDESTROY(x) SafeDelete(x)
#define tCHECKED_PTR(x) tCheckedPTR<x>
#define tCHECKED_PTR_CONST(x) tCheckedPTRConst<x>
#define tCHECK_DEST tCheckedPTRBase::CheckDestructed(this) 
#define tSAFEPTR
#else
#define tDESTROY(x) SafeDeletePtr(x)
#define tCHECK_DEST 
#define tCHECKED_PTR(x) x *
#define tCHECKED_PTR_CONST(x) const x *
#endif
#else
#define tDESTROY(x) SafeDeletePtr(x)
#define tCHECK_DEST 
#define tCHECKED_PTR(x) x *
#define tCHECKED_PTR_CONST(x) const x *
#endif

#define tCONTROLLED_PTR(x) tControlledPTR<x>



template<class T> class tControlledPTR{
    tCHECKED_PTR(T) target;

    void AddRef(){
        if (target)
            target->AddRef();
    }

    void Release(){
        if (target){
            //#ifdef tSAFEPTR
            T *dummy=reinterpret_cast<T*>(target);
            target=NULL;
            dummy->Release();
            //#else
            //      target->Release();
            //      target=NULL;
            //#endif
        }
    }

public:

    tControlledPTR(T *x):target(x){AddRef();}
    tControlledPTR(const tCheckedPTR<T> &x):target(x.operator->()){AddRef();}
    tControlledPTR(const tControlledPTR<T> &x):target(x.target){AddRef();}
    tControlledPTR():target(NULL){}


    tControlledPTR<T> &operator=(T *x){
        if (target!=x){
            Release();
            target=x;
            AddRef();
        }
        return *this;
    }

    tControlledPTR<T> &operator=(const tControlledPTR<T> &x){
        if (target!=x.target){
            Release();
            target=x.target;
            AddRef();
        }
        return *this;
    }

    T * operator->() const {
        return target;
    }

    T & operator*() const {
        return *target;
    }

    operator T*() const {
        return target;
    }

    /*
      T** operator&() {
        return &target;
      }
    */

    bool operator==(const T* x)const{
        return target==x;
    }

    bool operator!=(const T* x)const{
        return target!=x;
    }

    bool operator==(T* x)const{
        return target==x;
    }

    bool operator!=(T* x)const{
        return target!=x;
    }

    operator bool()const{
        return target!=NULL;
    }

    bool operator !()const{
        return target==NULL;
    }

    bool operator==(const tControlledPTR<T> &x)const{
        return target==x.target;
    }

    bool operator!=(const tControlledPTR<T> &x)const{
        return target!=x.target;
    }

    void Destroy(){
        if (target){
#ifdef tSAFEPTR
            T *dummy=(T *)target;
            target=NULL;
            delete dummy;
#else
            delete target();
            target=NULL;
#endif
        }
    }


    ~tControlledPTR(){
        Release();
    }
};

template<class T> class tJUST_CONTROLLED_PTR{
    T * target;

    void AddRef(){
        if (target)
            target->AddRef();
    }

    void Release(){
        if (target){
            T *dummy=target;
            target=NULL;
            dummy->Release();
        }
    }

public:

    tJUST_CONTROLLED_PTR(T *x):target(x){AddRef();}
    tJUST_CONTROLLED_PTR(const tCheckedPTR<T> &x):target(x.operator->()){AddRef();}
    tJUST_CONTROLLED_PTR(const tJUST_CONTROLLED_PTR<T> &x):target(x.target){AddRef();}
    tJUST_CONTROLLED_PTR():target(NULL){}


    tJUST_CONTROLLED_PTR<T> &operator=(T *x){
        if (target!=x){
            Release();
            target=x;
            AddRef();
        }
        return *this;
    }

    tJUST_CONTROLLED_PTR<T> &operator=(const tJUST_CONTROLLED_PTR<T> &x){
        operator=(x.target);
        return *this;
    }

    T * operator->() const {
        return target;
    }

    T & operator*() const {
        return *target;
    }

    operator T*() const {
        return target;
    }

    /*
      T** operator&() {
        return &target;
      }
    */

    bool operator==(const T* x)const{
        return target==x;
    }

    bool operator!=(const T* x)const{
        return target!=x;
    }

    bool operator==(const tJUST_CONTROLLED_PTR<T> &x)const{
        return target==x.target;
    }

    bool operator!=(const tJUST_CONTROLLED_PTR<T> &x)const{
        return target!=x.target;
    }

    void Destroy(){
        if (target){
#ifdef tSAFEPTR
            T *dummy=(T *)target;
            target=NULL;
            delete dummy;
#else
            delete target();
            target=NULL;
#endif
        }
    }


    ~tJUST_CONTROLLED_PTR(){
        Release();
    }
};


template<class T> bool operator==(const T *x, const tJUST_CONTROLLED_PTR<T> &y)
{
    return (x == static_cast<const T*>(y));
}

template<class T> bool operator==(T *x, const tJUST_CONTROLLED_PTR<T> &y)
{
    return (x == static_cast<const T*>(y));
}

template<class T> bool operator==(const T *x, tJUST_CONTROLLED_PTR<T> &y)
{
    return (x == static_cast<const T*>(y));
}

template<class T> bool operator==(T *x, tJUST_CONTROLLED_PTR<T> &y)
{
    return (x == static_cast<const T*>(y));
}

template< class T > class tStackObject: public T
{
public:
    tStackObject(){ this->AddRef(); }
    template< typename A >
    explicit tStackObject( A& a ): T( a ) { this->AddRef(); }
    template< typename A, typename B >
    tStackObject( A& a, B& b ): T( a, b ) { this->AddRef(); }
    template< typename A, typename B, typename C >
    tStackObject( A& a, B& b, C& c ): T( a, b, c ) { this->AddRef(); }

    ~tStackObject()
    {
        if ( this->GetRefcount() != 1 )
            st_Breakpoint();

        this->refCtr_ = -1000;
    }
};

#ifdef DEBUG
void st_AddRefBreakpint( void const * object );
void st_ReleaseBreakpint( void const * object );
#endif

// not thread-safe mutex
struct tNonMutex
{
    void acquire(){}
    void release(){}
};

template< class T, class MUTEX = tNonMutex > class tReferencable
{
    friend class tStackObject< T >;

public:
    tReferencable()												:refCtr_(0) {}
    tReferencable				( const tReferencable& )						:refCtr_(0) {}
    tReferencable& operator =	( const tReferencable& ){ return *this; }

    void		AddRef		()	const
    {
#ifdef DEBUG
        st_AddRefBreakpint( this );
#endif        
        tASSERT( this && refCtr_ >= 0 );
        mutex_.acquire();
        ++refCtr_;
        mutex_.release();
        tASSERT( this && refCtr_ >= 0 );
    }

    void		Release		() const
    {
#ifdef DEBUG
        st_ReleaseBreakpint( this );
#endif        

        tASSERT ( this && refCtr_ >= 0 );
        mutex_.acquire();
        --refCtr_;
        bool kill = (refCtr_ <= 0);
        mutex_.release();

        if ( kill )
        {
            refCtr_ = -1000;
            delete static_cast< const T* >( this );
        }
    }

    int			GetRefcount	() const
    {
        tASSERT( this );
        return refCtr_;
    }
protected:
    ~tReferencable()
    {
        tASSERT( this && ( refCtr_ == -1000 || refCtr_ == 0 ) );
        refCtr_ = -1000;
    }
private:
    mutable int refCtr_;
    mutable MUTEX mutex_;
};


#endif



