#include "tMemManager.h"
#include "tLinkedList.h"
#include "tSysTime.h"

#include <iostream>

#define MAX 100

class test3;

test3 *anchor;

class test3:public tListItem<test3>{
public:
    test3():tListItem<test3>(::anchor){};
};

test3 a,b,c,d;


class test{
    int x;

public:
    virtual ~test(){};

    /*
    #define classname test

    public:
    void *operator new(size_t s){
      if (sizeof(classname)>MAX_SIZE*4)
        return::operator new(s);
      else 
        return tMemMan::Alloc(sizeof(classname));
    }

    void operator delete(void *p){
      if (p) { 
        if (sizeof(classname)>MAX_SIZE*4)
    ::operator delete(p); 
        else 
    tMemMan::Dispose(p,sizeof(classname));
      }
    }
    */

    tMEMMANAGER(test);
};


class test2: public test{
    int y;

public:
    virtual ~test2(){};

    tMEMMANAGER(test2);
};



class A{
public:
    int x;

    A(int X):x(X){}
};

class B:virtual public A{
public:
    B():A(1){}
};

class C:public B{
public:
    C():A(2){}
};

#define LEN  100000
#define ELEM 3

void test_max_a(){
    float x[LEN][ELEM];
    int i,j;

    for (i=LEN-1; i>=0; i--)
    {
        x[i][0] = 100;
        x[i][1] = 150-i;
        x[i][2] = 75+i;
    }

    for (int k=99; k>=0; k--)
        for (i=LEN-1; i>=0; i--)
        {
            float max = -10000;
            float min = 10000;

            for (j=ELEM; j>=0; j--)
            {
                float y   = x[i][j];
                float ymi = min-y;
                float yma = y-max;

                min -= (ymi + fabs(ymi))*.5;
                max += (yma + fabs(yma))*.5;
                /*
                if (min > x[i][j])
                  min = x[i][j];
                else
                  min = min;

                if (max < x[i][j])
                  max = x[i][j];
                else
                  max = max;
                */
            }
        }
}

void test_max_b(){
    float x[LEN][ELEM];
    int i,j;

    for (i=LEN-1; i>=0; i--)
    {
        x[i][0] = 100;
        x[i][1] = 150-i;
        x[i][2] = 75+i;
    }

    for (int k=99; k>=0; k--)
        for (i=LEN-1; i>=0; i--)
        {
            float max = -10000;
            float min = 10000;

            for (j=ELEM; j>=0; j--)
            {
                min = (min > x[i][j] ? x[i][j] : min);
                max = (max < x[i][j] ? x[i][j] : max);
            }
        }
}



int main(){
    float ta = tSysTimeFloat();
    test_max_b();
    float tb = tSysTimeFloat();
    test_max_a();
    float tc = tSysTimeFloat();

    std::cout << tb-ta << " , " << tc-tb << '\n';



    C c;
    std::cout << c.x << '\n';

    int i;
    test *x[MAX];

    test *y=new test2;
    delete y;

    for(i=0;i<MAX;i++)
        x[i]=new test;

    for (i=0;i<MAX;i++)
        if (i%4 !=0){
            delete x[i];
            x[i]=NULL;
        }

    for(i=0;i<MAX;i++)
        if (!x[i])
            x[i]=new test;

    for(i=0;i<MAX;i++)
        delete x[i];

    return 0;
}





