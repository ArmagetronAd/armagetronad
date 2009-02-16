
#include "tFunction.h"

// *******************************************************************************
// *
// *	tFunction
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tFunction::tFunction( void )
        : offset_(0), slope_(0)
{
}

// *******************************************************************************
// *
// *	tFunction
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tFunction::tFunction(REAL offset, REAL slope)
        : offset_(offset), slope_(slope)
{
}

// *******************************************************************************
// *
// *	parse
// *
// *******************************************************************************
//!
//!		@param	argument
//!		@return
//!
// *******************************************************************************

tFunction &
tFunction::parse(std::string const & str, REAL const * addSizeMultiplier)
{
    REAL param[2] = {0.0, 0.0};
    int bPos;
    if ( (bPos = str.find(';')) != -1)
    {
        param[0] = atof(str.substr(0, bPos).c_str());
        param[1] = atof(str.substr(bPos + 1, str.length()).c_str());
    }
    else
    {
        param[0] = atof(str.c_str());
    }

    if (addSizeMultiplier)
    {
        REAL const & sizeMultiplier = *addSizeMultiplier;
        param[0] = param[0] * sizeMultiplier;
        param[1] = param[1] * sizeMultiplier;
    }
    tFunction & tf = *this;
    tf.SetOffset(param[0]);
    tf.SetSlope(param[1]);
    return tf;
}
tFunction &
tFunction::parse(std::string const & str, REAL const & sizeMultiplier)
{
    return
    parse(str, &sizeMultiplier);
}
tFunction &
tFunction::parse(std::string const & str)
{
    return
    parse(str, (REAL const *)NULL);
}

// *******************************************************************************
// *
// *	~tFunction
// *
// *******************************************************************************
//!
//!
// *******************************************************************************

tFunction::~tFunction( void )
{
}

// *******************************************************************************
// *
// *	Evaluate
// *
// *******************************************************************************
//!
//!		@param	argument
//!		@return
//!
// *******************************************************************************

REAL tFunction::Evaluate( REAL argument ) const
{
    return offset_ + slope_ * argument;
}
