#include "nMessageMock.h"
#include "tPolynomial.h"
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

class tPolynomialMarshalerTest : public CppUnit::TestFixture {
private:
    tPolynomialMarshaler marshalerA;
    tPolynomialMarshaler marshalerB;
    tPolynomialMarshaler marshalerC;

  std::string dataInA;
  std::string dataInB;
  std::string dataInC;

public:
    CPPUNIT_TEST_SUITE( tPolynomialMarshalerTest );
    CPPUNIT_TEST( testParsing );
    CPPUNIT_TEST( testConstructor );
    CPPUNIT_TEST( testMarshaling );
    CPPUNIT_TEST( testMarshalingWithBase );
    CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {
    dataInA = "3;5";
    dataInB = "3:5;7:9";
    dataInC = "1:2:3;4:5:6";
    // populate marshaler C to :
    // constant = {3.0}
    // variant  = {5.0}
    marshalerA.setConstant(3.0, 0);
    marshalerA.setVariant(5.0, 0);

    // populate marshaler C to :
    // constant = {3.0, 5.0}
    // variant  = {7.0, 9.0}
    marshalerB.setConstant(3.0, 0);
    marshalerB.setConstant(5.0, 1);
    marshalerB.setVariant(7.0, 0);
    marshalerB.setVariant(9.0, 1);

    // populate marshaler C to :
    // constant = {1.0, 2.0, 3.0}
    // variant  = {4.0, 5.0, 6.0}
    marshalerC.setConstant(1.0, 0);
    marshalerC.setConstant(2.0, 1);
    marshalerC.setConstant(3.0, 2);
    marshalerC.setVariant(4.0, 0);
    marshalerC.setVariant(5.0, 1);
    marshalerC.setVariant(6.0, 2);
  }

  void tearDown() {
    // Empty
  }

  void testParsing() {
    tPolynomialMarshaler marshalerParsed;

    marshalerParsed.parse(dataInB);
    CPPUNIT_ASSERT( marshalerB == marshalerParsed );

    marshalerParsed.parse(dataInA);
    CPPUNIT_ASSERT( marshalerA == marshalerParsed );

    marshalerParsed.parse(dataInC);
    CPPUNIT_ASSERT( marshalerC == marshalerParsed );
  }

  void testConstructor() {
    // copy constructor
    tPolynomialMarshaler marshalerCopyA(marshalerA);
    CPPUNIT_ASSERT( marshalerA == marshalerCopyA );
    tPolynomialMarshaler marshalerCopyB(marshalerB);
    CPPUNIT_ASSERT( marshalerB == marshalerCopyB );
    tPolynomialMarshaler marshalerCopyC(marshalerC);
    CPPUNIT_ASSERT( marshalerC == marshalerCopyC );

    // parsing constructor
    tPolynomialMarshaler marshalerParsedA(dataInA);
    CPPUNIT_ASSERT( marshalerA == marshalerParsedA );
    tPolynomialMarshaler marshalerParsedB(dataInB);
    CPPUNIT_ASSERT( marshalerB == marshalerParsedB );
    tPolynomialMarshaler marshalerParsedC(dataInC);
    CPPUNIT_ASSERT( marshalerC == marshalerParsedC );
  }

  void testMarshaling() {
    /*
     * the polynomial {7, 5, 3} (here noted as x) will be marshaled as:
     * a + b*x + c*x^2 + t * (d + e*x + f*x^2)
     *
     * For the case A:
     * a=3, d=5, b=c=e=f=0
     * should produce:
     * a + b*x + c*x^2 + t * (d + e*x + f*x^2)
     * a + t * (d)
     * 3 + 5t
     *
     * For the case B:
     * a=3, b=5, d=7, e=9, c=f=0
     * should produce:
     * a + b*x + c*x^2 + t * (d + e*x + f*x^2)
     * a + b*x + t * (d + e*x)
     * 3 + 5*x + t * (7 + 9*x)
     * 3 + 5*{7, 5, 3} + t * (7 + 9*{7, 5, 3})
     * 3 + {35, 25, 15t} + t * (7 + {63, 45, 27})
     * 3 + (35 + 25*t +15*t^2) + t * (7 + 63 + 45*t + 27*t^2})
     * 38 + 25*t +15*t^2 + t * (70 + 45*t + 27*t^2})
     * 38 + 25*t +15*t^2 + 70*t + 45*t^2 + 27*t^3
     * 38 + 95*t + 60*t^2 + 27*t^3
     *
     * For the case C:
     * where a=1, b=2, c=3, d=4, e=5, f=6
     * should produce:
     * a + b*x + c*x^2 + t * (d + e*x + f*x^2)
     * 1 + 2*x + 3*x^2 + t * (4 + 5*x + 6*x^2)
     * 1 + 2*{7, 5, 3} + 3*{7, 5, 3}^2 + t * (4 + 5*{7, 5, 3} + 6*{7, 5, 3}^2)
     * nota:  {7, 5, 3}^2 => {49, 70, 67, 30, 9}
     * 1 + 2*{7, 5, 3} + 3*{49, 70, 67, 30, 9} + t * (4 + 5*{7, 5, 3} + 6*{49, 70, 67, 30, 9})
     * 1 + {14, 10, 6} + {147, 210, 201, 90, 27} + t * (4 + {35, 25, 15} + {294, 420, 402, 180, 54})
     * 1 + 14 + 10*t + 6*t^2  +  147 + 210*t + 201*t^2 + 90*t^3 + 27*t^4  +  t * (4 + 35 + 25*t + 15*t^2  +  294 + 420*t + 402*t^2 + 180*t^3 + 54*t^4)
     * 162 + 220*t + 207*t^2 + 90*t^3 + 27*t^4  +  t * (333 + 445*t + 417*t^2 + 180*t^3 + 54*t^4)
     * 162 + 220*t + 207*t^2 + 90*t^3 + 27*t^4  +  333*t + 445*t^2 + 417*t^3 + 180*t^4 + 54*t^5
     * 162 + 553*t + 652*t^2 + 507*t^3 + 207*t^4 + 54*t^5
     * 
     */
    float inputData[] = {7, 5, 3};
    float expectedDataA[] = {3, 5};
    float expectedDataB[] = {38, 95, 60, 27};
    float expectedDataC[] = {162, 553, 652, 507, 207, 54};
    tPolynomial<nMessageMock> inputPolynomial(inputData, sizeof(inputData)/sizeof(inputData[0]));
    tPolynomial<nMessageMock> expectedA(expectedDataA, sizeof(expectedDataA)/sizeof(expectedDataA[0]));
    tPolynomial<nMessageMock> expectedB(expectedDataB, sizeof(expectedDataB)/sizeof(expectedDataB[0]));
    tPolynomial<nMessageMock> expectedC(expectedDataC, sizeof(expectedDataC)/sizeof(expectedDataC[0]));

    float tData[] = {0, 1};
    tPolynomial<nMessageMock> t( tData, 2 );

    /*
    // Similar to B
    tPolynomial<nMessageMock> out2 = 
      inputPolynomial*5
      + 3
      + t * (
	     inputPolynomial*9
	     + 7
	     );

    std::cout << std::endl;
    std::cout << "manually generated expected output for case B : " << std::endl;
    std::cout << out2.toString() << std::endl;
    std::cout << "marshaled output for case B : " << std::endl;
    std::cout << marshalerB.marshal(inputPolynomial).toString() << std::endl;
    */

    /*
    // Similar to C
    tPolynomial<nMessageMock> out3 = 
      inputPolynomial*inputPolynomial*3 
      + inputPolynomial*2
      + 1
      + t * (
	     inputPolynomial*inputPolynomial*6
	     + inputPolynomial*5
	     + 4
	     );


    std::cout << std::endl;
    std::cout << "manually generated expected output for case C : " << std::endl;
    std::cout << out3.toString() << std::endl;
    std::cout << "marshaled output for case C : " << std::endl;
    std::cout << marshalerC.marshal(inputPolynomial).toString() << std::endl;
    */

    CPPUNIT_ASSERT ( expectedA == marshalerA.marshal(inputPolynomial) );
    CPPUNIT_ASSERT ( expectedB == marshalerB.marshal(inputPolynomial) );
    CPPUNIT_ASSERT ( expectedC == marshalerC.marshal(inputPolynomial) );
  }

  /**
   * The same logical input, transformed to 2 different reference value, and then marshalled
   * should produce the same value at a given current value.
   */ 
  void testMarshalingWithBase()
  {
    float inputData[] = {7, 5, 3};
    tPolynomial<nMessageMock> inputPolynomialAt0(inputData, sizeof(inputData)/sizeof(inputData[0]));
    tPolynomial<nMessageMock> inputPolynomialAt3(inputPolynomialAt0.adaptToNewReferenceVarValue(3));

    tPolynomial<nMessageMock> marshaledResultAt0 = marshalerB.marshal(inputPolynomialAt0);
    tPolynomial<nMessageMock> marshaledResultAt3 = marshalerB.marshal(inputPolynomialAt3);

    //    CPPUNIT_ASSERT ( marshaledResultAt0 == marshaledResultAt3 );
    std::cout << "at 0 : " << marshaledResultAt0(5) << " at 3 " << marshaledResultAt3(5) << std::endl;
    CPPUNIT_ASSERT ( marshaledResultAt0(5) == marshaledResultAt3(5) );

  }

};

CPPUNIT_TEST_SUITE_REGISTRATION( tPolynomialMarshalerTest );
