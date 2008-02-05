/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is f.ree software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/ // LoadLibrary

// disable malloc replacement
#define NO_MALLOC_REPLACEMENT

#include "defs.h"

#include <iostream>
#include <sstream>
#include <stdio.h>  // need basic C IO since STL IO does memory management
#include "tMemManager.h"
#include "tError.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef WIN32
#ifdef _MSC_VER 
#include <windows.h>
#include <winuser.h>
#ifdef _DEBUG
#undef _DEBUG
#endif
#include <CrtDbg.h>
#pragma warning (disable : 4073)
#pragma init_seg(lib)
#endif
#ifdef __MINGW32__
#include <windows.h>
#include <winuser.h>
#ifdef _DEBUG
#undef _DEBUG
#endif
#endif
static CRITICAL_SECTION  mutex;
#endif

#ifndef DONTUSEMEMMANAGER
#define NEW
#endif

#ifdef DEBUG
#define LEAKFINDER


#ifdef LEAKFINDER
#define PROFILER
#endif

#ifdef WIN32
//#define MEM_DEB_SMALL
//#define MEM_DEB
//#define DOUBLEFREEFINDER
#endif

//#define MEM_DEB_SMALL
//#define MEM_DEB
//#define DOUBLEFREEFINDER
#endif

static bool reported=false;

#ifdef HAVE_LIBZTHREAD
#include <zthread/FastRecursiveMutex.h>

static ZThread::FastRecursiveMutex st_mutex;
#else
class tMockMutex
{
public:
    void acquire(){};
    void release(){};
};

static tMockMutex st_mutex;
#endif

class tBottleNeck
{
public:
    tBottleNeck()
    {
        st_mutex.acquire();
    }

    ~tBottleNeck()
    {
        st_mutex.release();
    }
};

// replacement for wmemset function
#ifndef HAVE_WMEMSET
static inline wchar_t *wmemset(wchar_t *wcs, wchar_t wc, size_t n) throw()
{
    // fill memory
    for( size_t i = 0; i < n; ++i )
    {
        wcs[i] = wc;
    }

    return wcs;
}
#endif

void leak(){
    if (!reported)
    {
        reported=true;
#ifdef DEBUG
#ifdef WIN32
        MessageBox (NULL, "Memory leak detected!" , "Memory Leak", MB_OK);
#else
        std::cerr << "\n\nMemory leak detected!\n\n";
#endif
#endif
        tMemManBase::Profile();
    }
}



#include "tList.h"

class memblock;

#ifdef DEBUG
#define SAFETYBYTES 16
#else
#define SAFETYBYTES 0
#endif
#define PAD 197

// information about allocation in process
struct tAllocationInfo
{
#ifdef LEAKFINDER
    const char *filename;
    const char *classname;
    int checksum;
    int line;
    bool array;
#endif

    tAllocationInfo( bool 
#ifdef LEAKFINDER
                     array_ 
#endif
        )
#ifdef LEAKFINDER
    : filename( "XXX" ), classname( "XXX" ), checksum(-1), line(0), array( array_ )
#endif
    {
    }
};

class tMemMan: public tMemManBase
{
public:
    static void *Alloc(tAllocationInfo const & info, size_t s);
    static void  Dispose(tAllocationInfo const & info, void *p);
    static void  DisposeButKeep(tAllocationInfo const & info, void *p);
};


class tMemManager{
    friend class memblock;
public:
    tMemManager(int size, int blocksize);
    tMemManager(int size);
    ~tMemManager();

    void *      Alloc( tAllocationInfo const & info );
    static void * AllocDefault(tAllocationInfo const & info, size_t size);
    static void Dispose(tAllocationInfo const & info, void *p, bool keep=false);
    void        complete_Dispose(memblock *m);
    void        Check(); // check integrity

private:
    int  Lower(int i){ // the element below i in the heap
        if(i%2==0)  // just to make sure what happens; you never know what 1/2 is..
            return i/2-1;
        else
            return (i-1)/2;
    }

    bool SwapIf(int i,int j); // swaps heap elements i and j if they are
    // in wrong order; only then, TRUE is returned.
    // i is assumed to lie lower in the heap than j.

    void SwapDown(int j); // swap element j as far down as it may go.
    void SwapUp(int i);   // swap element j as far up as it must.

    //#ifdef MEM_DEB
    void CheckHeap(); // checks the heap structure
    //#endif

static int  UpperL(int i){return 2*i+1;} // the elements left and
    static int  UpperR(int i){return 2*i+2;} // right above i

    void Insert(memblock *b);  // starts to manage object e
    void Remove(memblock *b);  // stops (does not delete e)
    memblock * Remove(int i);     // stops to manage object i, wich is returned.
    void Replace(int i);  // puts element i to the right place (if the
    // rest of the heap has correct order)
    void Replace(memblock *e);


    int size;
    int blocksize;

    tList<memblock, true> blocks;
    tList<memblock, true> full_blocks;

    int semaphore;
};

static bool inited=false;

#ifdef LEAKFINDER
#include <fstream>
#define MAXCHECKSUM 100001
static int counter[MAXCHECKSUM];
static int leaks[MAXCHECKSUM];
#ifdef PROFILER
static const char *checksum_filename[MAXCHECKSUM];
static const char *checksum_classname[MAXCHECKSUM];
static int         checksum_line[MAXCHECKSUM];
static int         checksum_blocks[MAXCHECKSUM];
static int         checksum_bytes[MAXCHECKSUM];
#endif


static char const *leakname="leak.log";
static bool checkleaks=true;
#endif

void begin_checkleaks(){
#ifdef LEAKFINDER
    checkleaks=true;
#endif
}

struct chunkinfo{
unsigned char pos:8;       // our position in the block
unsigned char occupied:1;  // flag
unsigned char size_in_dwords:7; // size
unsigned char next:8;      // next element of same state
unsigned char prev:8;      // ....
#ifdef LEAKFINDER
    int checksum;
    int counter;
    bool array;
#endif
#ifdef PROFILER
    size_t realSize;
#endif
};

// the heap elements.

class memblock{
public: // everything is local to this file anyway...

    int value;         // our value (the number of free slots).
    // lower values are lower in the heap.
    int  hID;      // the id in the heap

    tMemManager *myman;

    int size;
    unsigned char first_free;


    ~memblock(){
        if (myman) myman->blocks.Remove(this,hID);
        //tASSERT(myman->blocksize == value);
    }

    chunkinfo & chunk(int i){
        tASSERT(i>=0 && i<myman->blocksize);
        return *(
                   (chunkinfo *)(void *)
                   ( ( (char *)(this+1) )+(sizeof(chunkinfo)+size+SAFETYBYTES)*i)
               );
    }

    char * safety(int i){
        tASSERT(i>=0 && i<myman->blocksize);
        return (
                   ( ( (char *)(this+1) )+(sizeof(chunkinfo)+size+SAFETYBYTES)*i)
                   +sizeof(chunkinfo)+size );
    }

    void * data(int i){
        tASSERT(i>=0 && i<myman->blocksize);
        return  (void *)
                ( ( (char *)(this+1) )+(sizeof(chunkinfo)+size+SAFETYBYTES)*i + sizeof(chunkinfo));
    }


    memblock(tMemManager *man):value(man->blocksize),hID(-1),myman(man){
        //    std::cout << "new memblock..\n";
        size=man->size;
        myman->blocks.Add(this,hID);

        // TODO: init linked list

        for (int i=man->blocksize-1 ; i>=0 ; i--){
            if (i<man->blocksize-1)
                chunk(i).next=i+2;
            else
                chunk(i).next=0;

            if (i>0)
                chunk(i).prev=i;
            else
                chunk(i).prev=0;

            chunk(i).pos = i;
            chunk(i).occupied = false;
            chunk(i).size_in_dwords = size >> 2;

            for (int j=SAFETYBYTES-1;j>=0;j--)
                safety(i)[j]=(j ^ PAD);
        }

#ifdef MEM_DEB
        Check();
#endif

        first_free=1;
    }


    void *Alloc( tAllocationInfo const & info ){
#ifdef MEM_DEB_SMALL
        Check();
#endif


        tASSERT(value>0);
        value--;

        // TODO: faster using linked list

        int ret=-1;
        /*
          for (int i=myman->blocksize-1 ; i>=0 ; i--)
          if (!chunk(i).occupied){
          //	  chunk(i).occupied=true;
          ret=i;
          i=-1;
          //  return data(i);
          }*/


        ret=first_free-1;
        tASSERT(ret>=0);
        // tASSERT(_CrtCheckMemory());

        chunk(ret).occupied=true;
        first_free=chunk(ret).next;
        //  tASSERT(_CrtCheckMemory());
        if (first_free>0) chunk(first_free-1).prev=0;

        chunk(ret).next=chunk(ret).prev=0;
        //   tASSERT(_CrtCheckMemory());

#ifdef LEAKFINDER
        if (info.checksum >= 0){
            tASSERT(info.checksum < MAXCHECKSUM);

#ifdef PROFILER
            if (!counter[info.checksum]){
                checksum_filename [info.checksum] = info.filename;
                checksum_classname[info.checksum] = info.classname;
                checksum_line     [info.checksum] = info.line;
            }
            checksum_blocks[info.checksum]++;
            checksum_bytes[info.checksum]+=size;
            chunk(ret).realSize = size;
#endif
            chunk(ret).array=info.array;
            chunk(ret).checksum=info.checksum;
            chunk(ret).counter =++counter[info.checksum];

            static int count =0;
            if (checkleaks && counter[info.checksum] == leaks[info.checksum]){
                count ++;
                if (count <= 0)
                {
                    std::cerr << "one of the endless ignored objects that were leaky last time created!\n";
                }
                else if (count <= 1)
                {
                    std::cerr << "Object that was leaky last time created!\n";
                    st_Breakpoint();
                }
                else if (count <= 3)
                {
                    std::cerr << "One more object that was leaky last time created!\n";
                    st_Breakpoint();
                }
                else
                {
                    std::cerr << "one of the endless objects that were leaky last time created!\n";
                    //	    st_Breakpoint();
                }
            }
        }
#endif

#ifdef LEAKFINDER
#ifdef PROFILER
        chunk(ret).realSize = size;
#endif
        chunk(ret).array=info.array;
        chunk(ret).checksum=info.checksum;
#endif

#ifdef DEBUG
        wmemset(static_cast< wchar_t * >( data(ret) ), wchar_t(0xfedabeef), size / sizeof( wchar_t ));
#else
        wmemset(static_cast< wchar_t * >( data(ret) ), 0, size / sizeof( wchar_t ));
#endif

#ifdef MEM_DEB_SMALL
        Check();
#endif

        return data(ret);

        tASSERT(0); // we should never get here
        return NULL;
    }

    static memblock * Dispose(void *p, int &size, tAllocationInfo const & info, bool keep=false){
        chunkinfo &c=((chunkinfo *)p)[-1];
        size=c.size_in_dwords*4;

#ifdef PROFILER
        if (c.checksum >=0 && c.checksum < MAXCHECKSUM){
            checksum_blocks[c.checksum]--;
            checksum_bytes [c.checksum]-=c.realSize;
        }
#else
#ifdef LEAKFINDER
        tASSERT(c.checksum>=-1 && c.checksum < MAXCHECKSUM);
#endif
#endif

#ifndef WIN32 // grr. on windows, this assertion fails on calls from STL.
        tASSERT( c.array == info.array );
#endif

        if (size==0){
            bool o = c.occupied;
            // tASSERT(c.occupied);
            c.occupied=false;
            if (o) free(&c);
            return NULL;
        }

        tASSERT(c.occupied == 1);
        if (!c.occupied)
            return NULL;

#ifdef DEBUG
        wmemset(static_cast< wchar_t * >( p ), wchar_t(0xbadf00d), size / sizeof( wchar_t ));
#else
        wmemset(static_cast< wchar_t * >( p ), 0, size / sizeof( wchar_t ) );
#endif

        memblock *myblock=(
                              (memblock *)(void *)
                              (
                                  ( (char *)(void *)p ) -
                                  ( (sizeof(chunkinfo)+size+SAFETYBYTES)*c.pos
                                    + sizeof(chunkinfo) )
                              ) ) - 1;

        c.occupied=false;



#ifndef DOUBLEFREEFINDER
        if (keep)
            return NULL;

#ifdef WIN32
        EnterCriticalSection(&mutex);
#endif

        tASSERT(myblock -> value < myblock->myman->blocksize);
        c.prev=0;
        c.next=myblock->first_free;
        myblock->first_free=c.pos+1;
        if (c.next)   myblock->chunk(c.next-1).prev=c.pos+1;

        myblock->value++;
#endif

        // TODO:  use linked list
        return myblock;
    }

    static memblock * create(tMemManager *man){
        //tASSERT(_CrtCheckMemory());
        void *mem = malloc(man->blocksize * (SAFETYBYTES + man->size + sizeof(chunkinfo))
                           + sizeof(memblock));
        //  tASSERT(_CrtCheckMemory());
        return new(mem) memblock(man);
    }

    static void destroy(memblock * b){
        b->~memblock();
        free((void *)b);
    }

    void  Check(){
#ifdef DEBUG
        tASSERT (size == myman->size);

        tASSERT (value >=0);
        tASSERT (value <= myman->blocksize);

        for(int i=myman->blocksize-1;i>=0;i--){
            tASSERT(chunk(i).pos == i);

            for (int j=SAFETYBYTES-1;j>=0;j--){
                char a=safety(i)[j];
                char b=j^PAD;
                tASSERT(a==b);
            }
        }
#endif
    }

#ifdef LEAKFINDER
    void dumpleaks(std::ostream &s){
        for (int i=myman->blocksize-1;i>=0;i--)
            if (chunk(i).occupied && chunk(i).checksum >= 0 ){
                s << chunk(i).checksum << " " << chunk(i).counter << '\n';
                leak();
            }
    }
#endif
};





// ***********************************************





bool tMemManager::SwapIf(int i,int j){
    if (i==j || i<0) return false; // safety

    memblock *e1=blocks(i),*e2=blocks(j);
    if (e1->value > e2->value){
        Swap(blocks(i),blocks(j));
        e1->hID=j;
        e2->hID=i;
        return true;
    }
    else
        return false;

}

tMemManager::~tMemManager(){
#ifdef LEAKFINDER
    bool warn = true;
#ifdef HAVE_LIBZTHREAD
    warn = false;
#endif

    if (inited){
        // l???sche das ding
        std::ofstream lw(leakname);
        lw << "\n\n";
        tMemMan::Profile();
    }
#endif

#ifdef WIN32
    if (inited)
        DeleteCriticalSection(&mutex);
#endif

    inited = false;

    //tASSERT (full_blocks.Len() == 0);
    int i;
    for (i=blocks.Len()-1;i>=0;i--){
        if (blocks(i)->value==blocksize)
            memblock::destroy(blocks(i));
        else{
#ifdef LEAKFINDER
            if ( warn )
            {
                std::cout << "Memmanager warning: leaving block untouched.\n";
                warn = false;
            }
            std::ofstream l(leakname,std::ios::app);
            blocks(i)->dumpleaks(l);
#endif
        }
    }

    for (i=full_blocks.Len()-1;i>=0;i--){
        if (full_blocks(i)->value==blocksize)
            memblock::destroy(full_blocks(i));
        else{
#ifdef LEAKFINDER
            if ( warn )
            {
                std::cout << "Memmanager warning: leaving block untouched.\n";
                warn = false;
            }
            std::ofstream l(leakname,std::ios::app);
            full_blocks(i)->dumpleaks(l);
#endif
        }
    }

}

tMemManager::tMemManager(int s,int bs):size(s),blocksize(s){
    // std::cout << sizeof(chunkinfo);
    if (blocksize>255)
        blocksize=255;
    semaphore = 1;

    tBottleNeck neck;

#ifdef WIN32
    if (!inited)
        InitializeCriticalSection(&mutex);
#endif

    inited = true;
}

tMemManager::tMemManager(int s):size(s){//,blocks(1000),full_blocks(1000){
    tBottleNeck neck;

    inited = false;

    blocks.SetLen(0);
    full_blocks.SetLen(0);
    blocksize=16000/(s+sizeof(chunkinfo)+SAFETYBYTES);
    if (blocksize>252)
        blocksize=252;
    //  std::cout << sizeof(chunkinfo) << ',' << s << ',' << blocksize << '\n';
    semaphore = 1;

#ifdef LEAKFINDER
    if (size == 0){
        for ( int i = MAXCHECKSUM-1; i>=0; --i )
        {
            leaks[i] = 0;
            counter[i] = 0;
#ifdef PROFILER
            checksum_filename[i] = 0;
            checksum_classname[i] = 0;
            checksum_line[i] = 0;
            checksum_blocks[i] = 0;
            checksum_bytes[i] = 0;
#endif
        }

        std::ifstream l(leakname);
        while (l.good() && !l.eof()){
            int cs,ln;
            l >> cs >> ln;

            if (cs>=0 && cs < MAXCHECKSUM && (ln < leaks[cs] || leaks[cs] == 0))
                leaks[cs]=ln;
        }
    }
    if (!inited){
        // delete it
        std::ofstream lw(leakname);
        lw << "\n\n";
    }
#endif

#ifdef WIN32
    if (!inited)
        InitializeCriticalSection(&mutex);
#endif

    inited = true;
}

//#ifdef MEM_DEB
void tMemManager::CheckHeap(){
    for(int i=blocks.Len()-1;i>0;i--){
        memblock *current=blocks(i);
        memblock *low=blocks(Lower(i));
        if (Lower(UpperL(i))!=i || Lower(UpperR(i))!=i)
            tERR_ERROR_INT("Error in lower/upper " << i << "!");

        if (low->value>current->value)
            tERR_ERROR_INT("Heap structure corrupt!");
    }
}
//#endif

void tMemManager::SwapDown(int j){
    int i=j;
    // e is now at position i. swap it down
    // as far as it goes:
    do{
        j=i;
        i=Lower(j);
    }
    while(SwapIf(i,j)); // mean: relies on the fact that SwapIf returns -1
    // if i<0.

#ifdef MEM_DEB
    CheckHeap();
#endif
}

void tMemManager::SwapUp(int i){
#ifdef MEM_DEB
    //  static int su=0;
    //if (su%100 ==0 )
    // std::cout << "su=" << su << '\n';
    //  if (su > 11594 )
    // con << "su=" << su << '\n';
    // su ++;
#endif

    int ul,ur;
    bool goon=1;
    while(goon && UpperL(i)<blocks.Len()){
        ul=UpperL(i);
        ur=UpperR(i);
        if(ur>=blocks.Len() ||
                blocks(ul)->value < blocks(ur)->value){
            goon=SwapIf(i,ul);
            i=ul;
        }
        else{
            goon=SwapIf(i,ur);
            i=ur;
        }
    }

#ifdef MEM_DEB
    CheckHeap();
#endif

}

void tMemManager::Insert(memblock *e){
    blocks.Add(e,e->hID); // relies on the implementation of List: e is
    // put to the back of the heap.
    SwapDown(blocks.Len()-1); // bring it to the right place

#ifdef MEM_DEB
    CheckHeap();
#endif
}

void tMemManager::Remove(memblock *e){
    int i=e->hID;

    if(i<0 || this != e->myman)
        tERR_ERROR_INT("Element is not in this heap!");

    Remove(i);

#ifdef MEM_DEB
    CheckHeap();
#endif
}

void tMemManager::Replace(int i){
    if (i>=0 && i < blocks.Len()){
        if (i==0 || blocks(i)->value > blocks(Lower(i))->value)
            SwapUp(i);          // put it up where it belongs
        else
            SwapDown(i);

#ifdef MEM_DEB
        CheckHeap();
#endif
    }
}

void tMemManager::Replace(memblock *e){
    int i=e->hID;

    if(i<0 || this != e->myman)
        tERR_ERROR_INT("Element is not in this heap!");

    Replace(i);

#ifdef MEM_DEB
    CheckHeap();
#endif
}

memblock * tMemManager::Remove(int i){
    if (i>=0 && i<blocks.Len()){
        memblock *ret=blocks(i);

        blocks.Remove(ret,ret->hID);

        // now we have an misplaced element at pos i. (if i was not at the end..)
        if (i<blocks.Len())
            Replace(i);

#ifdef MEM_DEB
        CheckHeap();
#endif

        return ret;
    }
    return NULL;
}

void * tMemManager::Alloc( tAllocationInfo const & info ){
    semaphore--;
    if (semaphore<0 || size == 0){
        semaphore++;
        return AllocDefault(info, size);
    }

#ifdef WIN32
    EnterCriticalSection(&mutex);
#endif

    if (blocks.Len()==0) // no free space available. create new block...
        Insert(memblock::create(this));
    //tASSERT(_CrtCheckMemory());

    memblock *block=blocks(0);
    tASSERT (block->size == size);
    void *mem = block->Alloc( info );
    //tASSERT(_CrtCheckMemory());
    if (block->value == 0){
        Remove(block);
        full_blocks.Add(block,block->hID);
    }

#ifdef WIN32
    LeaveCriticalSection(&mutex);
#endif

    semaphore++;
    return mem;
}

void tMemManager::complete_Dispose(memblock *block){
#ifdef MEM_DEB_SMALL
    block->Check();
#endif

    semaphore--;
    if (semaphore>=0){
        if (block->value == 1){
            full_blocks.Remove(block,block->hID);
            Insert(block);
        }
        else{
            Replace(block);
        }
    }
    //else
    // tASSERT(0);
    semaphore++;

#ifdef MEM_DEB_SMALL
    block->Check();
#endif
}

void tMemManager::Check(){
#ifdef DEBUG_EXPENSIVE
    if ( !inited )
        return;

    CheckHeap();
    int i;
    for (i=blocks.Len()-1;i>=0;i--)
    {
        blocks(i)->Check();
        tASSERT (blocks(i)->myman == this);
        tASSERT (blocks(i)->size == size);
    }
    for (i=full_blocks.Len()-1;i>=0;i--)
    {
        if ( full_blocks(i) )
        {
            full_blocks(i)->Check();
            tASSERT (full_blocks(i)->myman == this);
            tASSERT (full_blocks(i)->size == size);
        }
    }
#endif
}


#define MAX_SIZE 109


static tMemManager memman[MAX_SIZE+1]={
                                          tMemManager(0),
                                          tMemManager(4),
                                          tMemManager(8),
                                          tMemManager(12),
                                          tMemManager(16),
                                          tMemManager(20),
                                          tMemManager(24),
                                          tMemManager(28),
                                          tMemManager(32),
                                          tMemManager(36),
                                          tMemManager(40),
                                          tMemManager(44),
                                          tMemManager(48),
                                          tMemManager(52),
                                          tMemManager(56),
                                          tMemManager(60),
                                          tMemManager(64),
                                          tMemManager(68),
                                          tMemManager(72),
                                          tMemManager(76),
                                          tMemManager(80),
                                          tMemManager(84),
                                          tMemManager(88),
                                          tMemManager(92),
                                          tMemManager(96),
                                          tMemManager(100),
                                          tMemManager(104),
                                          tMemManager(108),
                                          tMemManager(112),
                                          tMemManager(116),
                                          tMemManager(120),
                                          tMemManager(124),
                                          tMemManager(128),
                                          tMemManager(132),
                                          tMemManager(136),
                                          tMemManager(140),
                                          tMemManager(144),
                                          tMemManager(148),
                                          tMemManager(152),
                                          tMemManager(156),
                                          tMemManager(160),
                                          tMemManager(164),
                                          tMemManager(168),
                                          tMemManager(172),
                                          tMemManager(176),
                                          tMemManager(180),
                                          tMemManager(184),
                                          tMemManager(188),
                                          tMemManager(192),
                                          tMemManager(196),
                                          tMemManager(200),
                                          tMemManager(204),
                                          tMemManager(208),
                                          tMemManager(212),
                                          tMemManager(216),
                                          tMemManager(220),
                                          tMemManager(224),
                                          tMemManager(228),
                                          tMemManager(232),
                                          tMemManager(236),
                                          tMemManager(240),
                                          tMemManager(244),
                                          tMemManager(248),
                                          tMemManager(252),
                                          tMemManager(256),
                                          tMemManager(260),
                                          tMemManager(264),
                                          tMemManager(268),
                                          tMemManager(272),
                                          tMemManager(276),
                                          tMemManager(280),
                                          tMemManager(284),
                                          tMemManager(288),
                                          tMemManager(292),
                                          tMemManager(296),
                                          tMemManager(300),
                                          tMemManager(304),
                                          tMemManager(308),
                                          tMemManager(312),
                                          tMemManager(316),
                                          tMemManager(320),
                                          tMemManager(324),
                                          tMemManager(328),
                                          tMemManager(332),
                                          tMemManager(336),
                                          tMemManager(340),
                                          tMemManager(344),
                                          tMemManager(348),
                                          tMemManager(352),
                                          tMemManager(356),
                                          tMemManager(360),
                                          tMemManager(364),
                                          tMemManager(368),
                                          tMemManager(372),
                                          tMemManager(376),
                                          tMemManager(380),
                                          tMemManager(384),
                                          tMemManager(388),
                                          tMemManager(392),
                                          tMemManager(396),
                                          tMemManager(400),
                                          tMemManager(404),
                                          tMemManager(408),
                                          tMemManager(412),
                                          tMemManager(416),
                                          tMemManager(420),
                                          tMemManager(424),
                                          tMemManager(428),
                                          tMemManager(432),
                                          tMemManager(436)
                                      };


void tMemManager::Dispose(tAllocationInfo const & info, void *p, bool keep){
    int size;

    if (!p)
        return;

    memblock *block = memblock::Dispose(p,size,info, keep);
#ifndef DOUBLEFREEFINDER
    if (inited && block){
        tBottleNeck neck;
        memman[size >> 2].complete_Dispose(block);
#ifdef WIN32
        LeaveCriticalSection(&mutex);
#endif
    }
#else
    block = 0;
#endif
}

void *tMemManager::AllocDefault(tAllocationInfo const & info, size_t s){
#ifdef LEAKFINDER
    void *ret=malloc(s+sizeof(chunkinfo));//,classname,fileName,line);
    ((chunkinfo *)ret)->checksum=info.checksum;
    ((chunkinfo *)ret)->array=info.array;
#ifdef PROFILER
    ((chunkinfo *)ret)->realSize=s;
    if ( info.checksum >= 0 )
    {
        if (!counter[info.checksum]){
            checksum_filename [info.checksum] = info.filename;
            checksum_classname[info.checksum] = info.classname;
            checksum_line     [info.checksum] = info.line;
        }
        checksum_blocks[info.checksum]++;
        checksum_bytes[info.checksum]+=s;
    }
#endif
#else
    void *ret=malloc(s+sizeof(chunkinfo));
#endif
    ((chunkinfo *)ret)->size_in_dwords=0;
    ((chunkinfo *)ret)->occupied=true;
    ret = ((chunkinfo *)ret)+1;
    return ret;
}

void *tMemMan::Alloc(tAllocationInfo const & info, size_t s){
#ifndef DONTUSEMEMMANAGER
#ifdef MEM_DEB
#ifdef WIN32
    tASSERT(_CrtCheckMemory());
#endif
    Check();
#endif
    void *ret;
    if (inited && s < (MAX_SIZE << 2))
    {
        tBottleNeck neck;
        ret=memman[((s+3)>>2)].Alloc( info );
    }
    else
    {
        ret=tMemManager::AllocDefault(info, s);
    }
#ifdef MEM_DEB
    Check();
#ifdef WIN32
    tASSERT(_CrtCheckMemory());
#endif
#endif
    return ret;

#else
    return malloc(s);
#endif

}

void tMemMan::DisposeButKeep( tAllocationInfo const & info, void *p){
#ifndef DONTUSEMEMMANAGER
#ifdef MEM_DEB
#ifdef WIN32
    tASSERT(_CrtCheckMemory());
#endif
    Check();
#endif
    tMemManager::Dispose(info, p, true);
#ifdef MEM_DEB
    Check();
#ifdef WIN32
    tASSERT(_CrtCheckMemory());
#endif
#endif
#endif
}

void tMemMan::Dispose(tAllocationInfo const & info, void *p){
#ifndef DONTUSEMEMMANAGER
#ifdef MEM_DEB
#ifdef WIN32
    tASSERT(_CrtCheckMemory());
#endif
    Check();
#endif
    tMemManager::Dispose(info, p );
#ifdef MEM_DEB
    Check();
#ifdef WIN32
    tASSERT(_CrtCheckMemory());
#endif
#endif
#else
    free(p);
#endif
}


void tMemManBase::Check(){
    if (!inited)
        return;

#ifdef WIN32
    EnterCriticalSection(&mutex);
#endif

    for (int i=MAX_SIZE;i>=0;i--)
        memman[i].Check();

#ifdef WIN32
    LeaveCriticalSection(&mutex);
#endif
}

/*
	void* operator new	(unsigned int size) {	
		//return zmalloc.Malloc (size, "UNKNOWN", "zMemory.cpp", 49);
        // return malloc(size);
        return tMemManager::AllocDefault(size);
	};
	void  operator delete(void *ptr) {	
		if (ptr) tMemManager::Dispose(ptr);
        // zmalloc.Free(ptr);							 
	}; 
*/

#ifdef NEW
/*
void * operator new(
        unsigned int cb,
        int nBlockUse,
        const char * szFileName,
        int nLine
        )
{
    return operator new(cb, "xxx", szFileName, nLine);
}
*/

#ifdef WIN32
#ifdef DEDICATED
#ifdef DEBUG
#if 0
void * operator new(
    size_t size,
    int nBlockUse,
    const char * file,
    int l
)
{
#ifdef LEAKFINDER
    fileName=file;
    classname="XXX";
    line=l;

    checksum =
#ifndef PROFILER 
        size +
#endif 
        line * 19671;

    int c=1379;
    while (*file){
        checksum = (checksum + (*file)*c) % MAXCHECKSUM;
        c        = (c * 79              ) % MAXCHECKSUM;
        file++;
    }

#ifdef PROFILER
    while (checksum_fileName[checksum] && (checksum_fileName[checksum] != fileName || checksum_line[checksum] != line))
        checksum = (checksum+1) % MAXCHECKSUM;
#endif

#endif
    return tMemMan::Alloc(size);

    //      return _nh_malloc_dbg( cb, 1, nBlockUse, szFileName, nLine );
}
#endif
#endif
#endif
#endif


void* _cdecl operator new	(size_t size) THROW_BADALLOC{
    tAllocationInfo info( false );

#ifdef LEAKFINDER
#ifndef HAVE_LIBZTHREAD
    info.checksum = size;
#endif
#endif
    return tMemMan::Alloc(info, size);
}

void  _cdecl operator delete(void *ptr) THROW_NOTHING{
    tAllocationInfo info( false );

    if (ptr)
        tMemMan::Dispose(info, ptr);
}

void  operator delete(void *ptr,bool keep) THROW_NOTHING{
    tAllocationInfo info( false );

    if (ptr){
        if (keep)
            tMemMan::DisposeButKeep(info, ptr);
        else
            tMemMan::Dispose(info, ptr);
        
    }
}



void* operator new	(size_t size,const char *classn,const char *file,int l) THROW_BADALLOC{
    tAllocationInfo info( false );
#ifdef LEAKFINDER
    info.filename=file;
    info.classname=classn;
    info.line=l;

    info.checksum =
#ifndef PROFILER 
        size +
#endif 
        info.line * 19671;

    int c=1379;
    while (*file){
        info.checksum = (info.checksum + (*file)*c) % MAXCHECKSUM;
        c             = (c * 79              ) % MAXCHECKSUM;
        file++;
    }
    while (*classn){
        info.checksum = (info.checksum + (*classn)*c) % MAXCHECKSUM;
        c             = (c * 79                ) % MAXCHECKSUM;
        classn++;
    }

#ifdef PROFILER
    while (checksum_filename[info.checksum] && (checksum_filename[info.checksum] != info.filename || checksum_line[info.checksum] != info.line))
    info.checksum = (info.checksum+1) % MAXCHECKSUM;
#endif

#endif
    return tMemMan::Alloc(info, size);
}

void  operator delete   (void *ptr,const char *classname,const char *file,int line)  THROW_NOTHING{
    tAllocationInfo info( false );

    if (ptr) tMemMan::Dispose(info, ptr);
}

void* operator new[]	(size_t size) THROW_BADALLOC{
    tAllocationInfo info( true );
#ifdef LEAKFINDER
#ifndef HAVE_LIBZTHREAD
    info.checksum = size % MAXCHECKSUM;
#endif
#endif
    return tMemMan::Alloc(info, size);
}

void  operator delete[](void *ptr) THROW_NOTHING {
    tAllocationInfo info( true );

    if (ptr)
        tMemMan::Dispose(info, ptr);
}



void* operator new[]	(size_t size,const char *classn,const char *file,int l)  THROW_BADALLOC{
    tAllocationInfo info( true );
#ifdef LEAKFINDER
    info.filename=file;
    info.classname=classn;
    info.line=l;

    info.checksum =
#ifndef PROFILER 
    size +
#endif 
    info.line * 19671;
    
    int c=1379;
    while (*file){
        info.checksum = (info.checksum + (*file)*c) % MAXCHECKSUM;
        c             = (c * 79              ) % MAXCHECKSUM;
        file++;
    }
    while (*classn){
        info.checksum = (info.checksum + (*classn)*c) % MAXCHECKSUM;
        c             = (c * 79                ) % MAXCHECKSUM;
        classn++;
    }
    
#ifdef PROFILER
    while (checksum_filename[info.checksum] && (checksum_filename[info.checksum] != info.filename || checksum_line[info.checksum] != info.line))
    info.checksum = (info.checksum+1) % MAXCHECKSUM;
#endif

#endif
    return tMemMan::Alloc(info, size);
}

void  operator delete[]   (void *ptr,const char *classname,const char *file,int line)   THROW_NOTHING{
    tAllocationInfo info( true );

    if (ptr) tMemMan::Dispose(info, ptr);
}

#endif




#ifdef PROFILER
    static int compare(const void *a, const void *b){
        int A = checksum_bytes[*((const int *)a)];
        int B = checksum_bytes[*((const int *)b)];

        if (A<B)
            return -1;
        if (A>B)
            return 1;

        return 0;
    }

//#define PRINT(blocks, size, c, f, l, cs) s << std::setw(6) << blocks << "\t" << std::setw(9) << size << "\t" << std::setw(20) <<  c << "\t" << std::setw(70) <<  f << "\t" << std::setw(6) << l << "\t" << cs << "\n"

//#define PRINT(blocks, size, c, f, l, cs) fprintf("%d:6% std::setw(6) << blocks << "\t" << std::setw(9) << size << "\t" << std::setw(20) <<  c << "\t" << std::setw(70) <<  f << "\t" << std::setw(6) << l << "\t" << cs << "\n"

#endif


#ifdef WIN32
#define snprintf _snprintf
#endif

void  tMemManBase::Profile(){
    tBottleNeck neck;
#ifdef PROFILER

    int sort_checksum[MAXCHECKSUM];
    int size=0,i;

    for (i=MAXCHECKSUM-1;i>=0;i--)
        if (checksum_blocks[i])
            sort_checksum[size++] = i;

    qsort(sort_checksum, size, sizeof(int), &compare);

    static int num=1;

    char name[100];
    snprintf( name, 99, "memprofile%d.txt", num++ );
    //    std::ostringstream fn;

    //    fn << "memprofile" << num++ << ".txt" << '\0';


    FILE *f = fopen( name, "w" );
    //	fprintf( f, "%d\t%d\t%d\t%s\t%s\t%d\t%d\t\n", )

    int total_blocks=0,total_bytes =0;

    for (i=size-1; i>=0; i--)
    {
        int cs = sort_checksum[i];
        const char *fn = checksum_filename[cs];
        const char *cn = checksum_classname[cs];

        if (!fn)
            fn = "XXX";
        if (!cn)
            cn = "XXX";

        fprintf( f, "%d\t%d\t%30s\t%80s\t%d\t%d\t\n", checksum_blocks[cs], checksum_bytes[cs],cn, fn , checksum_line[cs], cs );
        //        PRINT(checksum_blocks[cs], checksum_bytes[cs],cn, fn , checksum_line[cs], cs);
        total_blocks += checksum_blocks[cs];
        total_bytes += checksum_bytes[cs];
    }

    fclose( f );


    /*
        std::ofstream s( name );
        s.setf(std::ios::left);

        PRINT("Blocks", "Bytes", "Class", "File", "Line", "Checksum");

        int total_blocks=0,total_bytes =0;

        for (i=size-1; i>=0; i--)
    	{
            int cs = sort_checksum[i];
            const char *fn = checksum_fileName[cs];
            const char *cn = checksum_classname[cs];

            if (!fn)
                fn = "XXX";
            if (!cn)
                cn = "XXX";

            PRINT(checksum_blocks[cs], checksum_bytes[cs],cn, fn , checksum_line[cs], cs);     
            total_blocks += checksum_blocks[cs];
            total_bytes += checksum_bytes[cs];
        }

        s << "\n\n";
        PRINT (total_blocks, total_bytes, "Total" , "", "", "");
    */
#endif
}

#ifdef HAVE_LIBEFENCE
class eFence
{
public:
    eFence()
    {
        EF_newFrame();
    }
    ~eFence()
    {
        EF_delFrame();
    }
};

static eFence fence;
#endif

#ifndef DONTUSEMEMMANAGER
void * real_calloc( size_t nmemb, size_t size ) // call calloc
{
    return calloc( nmemb, size );
}

void * real_malloc( size_t size )                // call malloc
{
    return malloc( size );
}

void   real_free( void * ptr )                   // call free
{
    free( ptr );
}

void * real_realloc( void * ptr, size_t size )   // call realloc
{
    return realloc( ptr, size );
}

char * real_strdup( char const * ptr )   // call strdup
{
    return strdup( ptr );
}

void * real_mmove( void * ptr, size_t size )     // take memory allocated by real_malloc or a C library function and move to managed memory
{
    // create new memory, copy, free old memory
    void * ret = tNEW(char)[size];
    memcpy( ret, ptr, size );
    real_free( ptr );

    return ret;
}

char * real_strmove( char * ptr )                // take C string allocated by real_malloc or a C library function and move to managed memory
{
    if ( ptr )
        return static_cast< char * >( real_mmove( ptr, strlen( ptr ) + 1 ) );
    else
        return 0;
}

#endif // DONTUSEMEMMANAGER

char * tStrDup( char const * s )
{
    return real_strmove( strdup( s ) );
}
