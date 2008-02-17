#include "tPolynomial.h"

void tPolynomialMarshaler::parse(std::string str)
{
    int bPos;
    if ( (bPos = str.find(';')) != -1)
    {
        parsePart(str.substr(0, bPos)               , constants);
        parsePart(str.substr(bPos + 1, str.length()), variants);
    }
    else
    {
        parsePart(str, constants);
        variants.SetLen(0);
    }
}

tPolynomialMarshaler::tPolynomialMarshaler() 
:constants(0),
  variants(0)
{
  // Empty
}

tPolynomialMarshaler::tPolynomialMarshaler(std::string str)
:constants(0),
  variants(0)
{
  parse(str);
}

tPolynomialMarshaler::tPolynomialMarshaler(const tPolynomialMarshaler & other)
:constants(other.constants),
  variants(other.variants)
{
  // Empty
}
    
void tPolynomialMarshaler::setConstant(REAL value, int index)
{
  if(index >= constants.Len()) {
    grow(constants, index+1);
  }
  constants[index] = value;
}

void tPolynomialMarshaler::setVariant(REAL value, int index)
{
  if(index >= variants.Len()) {
    grow(variants, index+1);
  }
  variants[index] = value;
}

void tPolynomialMarshaler::parsePart(std::string str, tArray<REAL> &array)
{
    int pos;
    int prevPos = 0;
    int index = 0;

    pos = str.find(':', 0);
    if(-1 != pos) {
      do{
	REAL value = atof(str.substr(prevPos, pos).c_str());
	array.SetLen(index + 2); // +1 because to write at index n, the len must be n+1. +1 to allocate a place for the element after the last ':'
	array[index] = value;
	
	prevPos = pos + 1;
	index ++;
      }
      while ( (pos = str.find(':', prevPos)) != -1) ;

      array[index] = atof(str.substr(prevPos, pos).c_str());

    }
    else {
      array.SetLen(1);
      array[0] = atof(str.c_str());
    }
}

bool areArrayEqual (const tArray<REAL> & left, const tArray<REAL> & right)
{
  bool res = true;
  if(left.Len() != right.Len())
    res = false;

  for(int i=0; i<left.Len(); i++) {
    if(fabs(left[i] - right[i]) > DELTA)
      res = false;
  }

  return res;
}

bool operator == (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right)
{
  bool res = true;
  
  res = areArrayEqual(left.constants, right.constants) 
     && areArrayEqual(left.variants, right.variants);
  
  return res;
}

bool operator != (const tPolynomialMarshaler & left, const tPolynomialMarshaler & right)
{
  return !(left == right);
}

void tPolynomialMarshaler::grow(tArray<REAL> &array, int newLength)
{
  int oldLength = array.Len();
  array.SetLen(newLength);
  for(int i=oldLength; i<newLength; i++) {
    array[i] = 0;
  }
}


std::string tPolynomialMarshaler::toString() const
{
  std::ostringstream ostr("");

  ostr << "Constant len :" << constants.Len() << " ";
  for(int i=0; i<constants.Len(); i++) {
    ostr << " c[" << i << "]:" << constants[i];
  }
  ostr << std::endl;
  ostr << "Variant  len :" << variants.Len() << " ";
  for(int i=0; i<variants.Len(); i++) {
    ostr << " c[" << i << "]:" << variants[i];
  }
  ostr << std::endl;

  return ostr.str();
}
