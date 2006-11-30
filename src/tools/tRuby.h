#ifndef _TRUBY_H_
#define _TRUBY_H_

#include "aa_config.h"

#ifdef HAVE_LIBRUBY

#include <ruby.h>
#include <intern.h>
#include <stdexcept>
#include <string>
#include "tDirectories.h"

namespace tRuby
{
/**
	\brief	A helper function for rb_protect calls to raise a C++ exception
*/
void CheckStatus(int status);

/**
       \brief  Initializes the ruby intepreter for use
   */
void InitializeInterpreter();

/**
    \brief  Cleans up the interpreter. Don't use it after calling this.
*/
void CleanupInterpreter();

/**
    \brief  Require the file into the main namespace. A runtime_error
            will be throw if there is a problem.
    \param  file    the file to require (example: "set")
*/
void Require(const char * file);

/**
	\brief
	\param	path		
	\param	filename	
	\param	wrap		
*/
void Load(const tPath & path, const char * filename, bool wrap=false);

/**
	\brief	Evaluates the ruby code in +code+ and returns the result
	
	A runtime_error will be thrown if an exception is raised by Ruby
	
	\param	code	the ruby code to be evaluated
*/
VALUE Eval(std::string code);

/**
	\class Sandbox
	\brief	A segemented object to evaluated code in
	
	A Sanbox is a segemented object in which evaluated code, loaded files,
	and required libraries will not pollute the main namespace. Sandbox
	*does not* allow evaluation of pottentially dangerous and untrusted code.
*/
class Sandbox
{
public:
    /**
    	\brief	Create a time new sandbox
    	\param	timeout		Evaluations are killed after this many seconds
    */
    Sandbox(float timeout=0);

    virtual ~Sandbox();

    /**
    	\brief	Evaluate the ruby code in +code+ in this sandbox
    	
    	A runtime_error will be thrown if an exception is raised by Ruby
    	
    	\param	code	the ruby code to be evaluated
    	\return	The ruby VALUE returned from the evaluation. Some values
                      can not be marshalled out of the sandbox, and in this case
                      an error will be thrown.
    */
    VALUE Eval(std::string code);

    /**
              \brief  Import a constant into this sandbox.

    	A runtime_error will be thrown if an exception is raised by Ruby
    	
              \param  constant    The constant to import (example: "Foo::Bar")
          */
    void Import(const char * constant);

    /**
    	\brief	Loads a file into this sandbox
    	
    	A runtime_error will be thrown if an exception is raised by Ruby
    	
    	\param	path
    	\param	filename
    */
    void Load(const tPath & path, const char * filename);
protected:
    VALUE sandbox_;
    VALUE & GetSandbox();
    static VALUE EvalProtect(VALUE data);
    static VALUE ConstGetProtect(VALUE file);
    static VALUE LoadProtect(VALUE file);
};

/**
	\brief	A segemented object to evaluate untrusted code in
	
	Safe allows the evaluation of potentially unsafe code. *Do not* take
	this for granted.
*/
class Safe : public Sandbox
{
public:
    Safe(float timeout=0);
    virtual ~Safe();
};
}
#endif // HAVE_LIBRUBY

#endif // ! _TRUBY_H_
