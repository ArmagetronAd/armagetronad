#include "nMessageMock.h"
#include "tPolynomial.h"
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

class tPolynomialTest : public CppUnit::TestFixture {
private:
    tPolynomial<nMessageMock> *tpEmpty, *tpOne, *tpZero, *tpTwo;

public:
    CPPUNIT_TEST_SUITE( tPolynomialTest );
    CPPUNIT_TEST( testEquality );
    CPPUNIT_TEST( testAddition );
    CPPUNIT_TEST( testMultiplication );
    CPPUNIT_TEST( testEvaluateAndBaseArgument );
    CPPUNIT_TEST( testWriteAndReadToStream );
    CPPUNIT_TEST_SUITE_END();

private:
    tPolynomial<nMessageMock> *tpEmpty, *tpOne, *tpZero, *tpTwo;

public:
    void setUp() {
        tpEmpty = new tPolynomial<nMessageMock>();
        tpZero  = new tPolynomial<nMessageMock>(0.0);
        tpOne   = new tPolynomial<nMessageMock>(1.0);
        tpTwo   = new tPolynomial<nMessageMock>(2.0);
    }

    void tearDown() {
        delete tpEmpty;
        delete tpZero;
        delete tpOne;
        delete tpTwo;
    }

    void testEquality() {
        CPPUNIT_ASSERT( *tpEmpty == *tpEmpty );
        CPPUNIT_ASSERT( *tpOne == *tpOne );
        CPPUNIT_ASSERT( *tpEmpty == *tpZero );
        CPPUNIT_ASSERT( !(*tpZero == *tpOne) );
        // testing the copy constructor
        CPPUNIT_ASSERT( *tpOne == tPolynomial<nMessageMock>(*tpOne) );
    }

    void testAddition() {
        // {0} = {-} + {0}
        CPPUNIT_ASSERT( *tpZero == *tpEmpty + *tpZero );
        // {1} = {0} + {1}
        CPPUNIT_ASSERT( *tpOne == *tpZero + *tpOne );
        // {2} = {1} + {1}
        CPPUNIT_ASSERT( *tpTwo == *tpOne + *tpOne );
        // a = b + c
        float a[] = {3, 5, 7, 11};
        float b[] = {0, 5, 3, -5};
        float c[] = {3, 0, 4, 16};
        tPolynomial<nMessageMock> tpA(a);
        tPolynomial<nMessageMock> tpB(b);
        tPolynomial<nMessageMock> tpC(c);

        CPPUNIT_ASSERT( tpA == tpB + tpC );

        // {0} = {-} + 0
        CPPUNIT_ASSERT( *tpZero == *tpEmpty + 0.0 );
        // {1} = {0} + 1
        CPPUNIT_ASSERT( *tpOne == *tpZero + 1.0 );
        // {2} = {1} + 1
        CPPUNIT_ASSERT( *tpTwo == *tpOne + 1.0 );

        // {a} + 1 == ({b} + 2) + {c} + -1
        CPPUNIT_ASSERT( (tpA + 1.0) == (tpB + 2.0) + tpC + -1.0);
    }

    void testMultiplication() {
        // {0} = {-} * {0}
        CPPUNIT_ASSERT( *tpZero == *tpEmpty * *tpZero );
        // {0} = {0} * {1}
        CPPUNIT_ASSERT( *tpZero == *tpZero * *tpOne );
        // {1} = {1} * {1}
        CPPUNIT_ASSERT( *tpOne == *tpOne * *tpOne );

        // a = b * c
        float a[] = {3.0, 2.5, -6.0, -17, -5, 12, 12};
        float b[] = {1, 0.0, -2, -2};
        float c[] = {3, 2.5, 0, -6};
        tPolynomial<nMessageMock> tpA(a);
        tPolynomial<nMessageMock> tpB(b);
        tPolynomial<nMessageMock> tpC(c);

        CPPUNIT_ASSERT( tpA == tpB * tpC );

        // {0} = {-} * 0
        CPPUNIT_ASSERT( *tpZero == *tpEmpty * 0.0 );
        // {0} = {0} * 1
        CPPUNIT_ASSERT( *tpZero == *tpZero * 1.0 );
        // {1} = {1} * 1
        CPPUNIT_ASSERT( *tpOne == *tpOne * 1.0 );

        // {a} * -2 == ({b} * 2) * ({c} * -1)
        CPPUNIT_ASSERT( (tpA * -2.0) == (tpB * 2.0) * (tpC * -1.0));
    }

#define DELTA 1e-10

    void testEvaluateAndBaseArgument() {
        float accelA = 10.0;
        float accelB = -10.0;
        float a[] = {0, 0, accelA};
        float b[] = {50, 45, accelB};
        tPolynomial<nMessageMock> tfA(a);
        tPolynomial<nMessageMock> tfB(b);

        CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, tfA.evaluate(0), DELTA);
        CPPUNIT_ASSERT_DOUBLES_EQUAL( 125.0, tfA.evaluate(5), DELTA);

        CPPUNIT_ASSERT_DOUBLES_EQUAL( 50.0, tfB.evaluate(0), DELTA);
        CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, tfB.evaluate(10), DELTA);

        // change the base argument and adjust the coefs to time 5
        float NEW_REFERENCE_TIME = 5.0;
        tfA.reevaluateCoefsAt(NEW_REFERENCE_TIME);
        tfB.reevaluateCoefsAt(NEW_REFERENCE_TIME);

        // These should still be true
        CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, tfA.evaluate(0), DELTA);
        CPPUNIT_ASSERT_DOUBLES_EQUAL( 125.0, tfA.evaluate(5), DELTA);

        CPPUNIT_ASSERT_DOUBLES_EQUAL( 50.0, tfB.evaluate(0), DELTA);
        CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.0, tfB.evaluate(10), DELTA);

        // changing the acceleration
        tfA.changeRate(2.0*accelA, 2, NEW_REFERENCE_TIME);
        tfB.changeRate(2.0*accelB, 2, NEW_REFERENCE_TIME);

        CPPUNIT_ASSERT_DOUBLES_EQUAL( 125.0 + 50*0 + 20/2.0*0*0, tfA.evaluate(5), DELTA);
        CPPUNIT_ASSERT_DOUBLES_EQUAL( 125.0 + 50*5 + 20/2.0*5*5, tfA.evaluate(10), DELTA);

        CPPUNIT_ASSERT_DOUBLES_EQUAL( 150.0 - 5*0 - 20.0/2.0*0*0, tfB.evaluate(5), DELTA);
        CPPUNIT_ASSERT_DOUBLES_EQUAL( 150.0 - 5*5 - 20.0/2.0*5*5, tfB.evaluate(10), DELTA);
    }

    void testWriteAndReadToStream() {
        float a[] = {1.0, 2.0, 10.0};
        float b[] = {50, 45, -10.0};
        tPolynomial<nMessageMock> tfA(a);
        tPolynomial<nMessageMock> tfB(b);
        tPolynomial<nMessageMock> tfC;

        nMessageMock messageMock;

        CPPUNIT_ASSERT( !(tfA == tfB) );

        messageMock << tfA;
        messageMock.receive();
        messageMock >> tfB;

        CPPUNIT_ASSERT( tfA == tfB );
    }


};

CPPUNIT_TEST_SUITE_REGISTRATION( tPolynomialTest );
