#include "nNetwork.h"
#include "tPolynomial.h"
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <cppunit/extensions/HelperMacros.h>

class tPolynomialTest : public CppUnit::TestFixture { 
private:
  tPolynomial<nMessage> *tpEmpty, *tpOne, *tpZero, *tpTwo;

public: 
  CPPUNIT_TEST_SUITE( tPolynomialTest );
  CPPUNIT_TEST( testEquality );
  CPPUNIT_TEST( testAddition );
  CPPUNIT_TEST_SUITE_END();

  CPPUNIT_TEST_SUITE_REGISTRATION( tPolynomialTest );

  void setUp(void) {
    tpEmpty = new tPolynomial<nMessage>();
    tpZero = new tPolynomial<nMessage>(0.0);
    tpOne  = new tPolynomial<nMessage>(1.0);
    tpTwo  = new tPolynomial<nMessage>(2.0);
  }

  void tearDown(void) {
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
  }

  void testAddition() {
    // 0 = 0 + 0
    CPPUNIT_ASSERT( *tpZero == *tpEmpty + *tpZero );
    // 1 = 0 + 1
    CPPUNIT_ASSERT( *tpOne == *tpZero + *tpOne );
    // 2 = 1 + 1
    CPPUNIT_ASSERT( *tpTwo == *tpOne + *tpOne );

  }
};
