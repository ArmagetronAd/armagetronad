#include "tRuby.h"

#ifdef HAVE_LIBRUBY

#ifdef MACOSX_XCODE
#   include "AARuby.h"
#endif

namespace tRuby
{
	std::string GetExceptionInfo();
	VALUE RequireProtect(VALUE lib);
	VALUE LoadProtect(VALUE data);
	VALUE EvalProtect(VALUE code);
	
	/*#############################################################
	  ###########################################################*/
		
    void InitializeInterpreter()
	{
        ruby_init();
#ifdef MACOSX_XCODE
        AARuby_init_loadpath();
#else
        ruby_init_loadpath();
#endif
        ruby_script("Armagetron Advanced");
	}
    
    void CleanupInterpreter()
	{
		ruby_finalize();
	}
	    
    void Require(const char * lib)
	{
		int status = 0;
		rb_protect(RequireProtect, reinterpret_cast<VALUE>(lib), &status);
		CheckStatus(status);
	}
    
	void Load(const tPath & path, const char * filename, bool w)
	{
		VALUE wrap = w ? Qtrue : Qfalse;
        tString realPath = path.GetReadPath(filename);
        if (realPath == "")
            return;

        VALUE args[] = {rb_str_new2(realPath.c_str()), wrap};
		int status = 0;
		rb_protect(LoadProtect, reinterpret_cast<VALUE>(args), &status);
		CheckStatus(status);
	}
		
	VALUE Eval(std::string code)
	{
		int status = 0;
		VALUE result = rb_protect(EvalProtect, reinterpret_cast<VALUE>(code.c_str()), &status);
		CheckStatus(status);
        return result;
	}
	
	/*#############################################################
	  ###########################################################*/
	
	std::string GetExceptionInfo()
    {
        VALUE exception = rb_gv_get("$!");
		if (NIL_P(exception))
			return std::string("No exception");
		
        VALUE klass = rb_funcall(rb_funcall(exception,
                                            rb_intern("class"),
                                            0),
                                 rb_intern("to_s"),
                                 0);
        VALUE to_s = rb_funcall(exception, rb_intern("to_s"), 0);
        VALUE backtrace = rb_funcall(rb_funcall(exception,
                                                rb_intern("backtrace"),
                                                0),
                                     rb_intern("join"),
                                     1,
                                     rb_str_new2("\n"));
        std::string info;
        info += StringValuePtr(klass);
        info += ": ";
        info += StringValuePtr(to_s);
        info += '\n';
        info += StringValuePtr(backtrace);
        return info;
    }
    
	void CheckStatus(int status)
	{
		if (status)
		{
			std::runtime_error error(GetExceptionInfo());
			throw error;
		}
	}
	
	VALUE RequireProtect(VALUE lib)
	{
		rb_require(reinterpret_cast<char *>(lib));
        return Qnil;
	}
	
	VALUE LoadProtect(VALUE data)
	{
		VALUE * args = reinterpret_cast<VALUE *>(data);
		rb_funcall(rb_mKernel, rb_intern("load"), 1, args[0], args[1]);
		return Qnil;
	}
	
	VALUE EvalProtect(VALUE code)
	{
		return rb_eval_string(reinterpret_cast<char *>(code));;
	}
	
	/*#############################################################
	  ###########################################################*/
	
	Sandbox::Sandbox(float timeout)
	{
        VALUE options = rb_hash_new();
        rb_funcall(options, rb_intern("[]="), 2, ID2SYM(rb_intern("init")), ID2SYM(rb_intern("all")));
        
        if (timeout != 0)
        {
            rb_funcall(options, rb_intern("[]="), 2, ID2SYM(rb_intern("timeout")), rb_float_new(timeout));
        }
        
		VALUE args[] = { rb_cObject, rb_str_new2("Sandbox") };
		int status = 0;
		VALUE cSandbox = rb_protect(ConstGetProtect, reinterpret_cast<VALUE>(args), &status);
		CheckStatus(status);
		
		sandbox_ = rb_funcall(cSandbox,
                              rb_intern("new"),
                              1,
                              options);
	}
	
	Sandbox::~Sandbox()
	{
	}
	
	VALUE Sandbox::Eval(std::string code)
	{
        int status = 0;
        VALUE args[] = { reinterpret_cast<VALUE>(this), rb_str_new2(code.c_str()) };
        VALUE val = rb_protect(EvalProtect, reinterpret_cast<VALUE>(args), &status);
        CheckStatus(status);
        return val;
	}
       
	void Sandbox::Import(const char * c)
	{
		VALUE constants = rb_funcall(rb_str_new2(c), rb_intern("split"), 1, rb_str_new2("::"));
        VALUE constant  = rb_cObject;
        for(int index = 0; index < RARRAY(constants)->len; ++index)
        {
			int status = 0;
			VALUE args[] = {constant, RARRAY(constants)->ptr[index]};
			constant = rb_protect(ConstGetProtect, reinterpret_cast<VALUE>(args), &status);
			CheckStatus(status);
        }
        rb_funcall(sandbox_, rb_intern("import"), 1, constant);
	}
	
	void Sandbox::Load(const tPath & path, const char * filename)
	{
		tString realPath = path.GetReadPath(filename);
        if (realPath == "")
            return;
        
        VALUE args[] = { reinterpret_cast<VALUE>(this), rb_str_new2(realPath.c_str()) };
        int status = 0;
        rb_protect(LoadProtect, reinterpret_cast<VALUE>(args), &status);
        CheckStatus(status);
	}
	
	/*#############################################################
	  ###########################################################*/
	
	VALUE & Sandbox::GetSandbox()
	{
		return sandbox_;
	}
	
	VALUE Sandbox::EvalProtect(VALUE data)
	{
		VALUE * args = reinterpret_cast<VALUE *>(data);
		Sandbox * sandbox = reinterpret_cast<Sandbox *>(args[0]);
		return rb_funcall(sandbox->GetSandbox(), rb_intern("eval"), 1, args[1]);
	}
	
	VALUE Sandbox::ConstGetProtect(VALUE data)
	{
		VALUE * args = reinterpret_cast<VALUE *>(data);
		return rb_funcall(args[0], rb_intern("const_get"), 1, args[1]);
	}
	
	VALUE Sandbox::LoadProtect(VALUE data)
	{
		VALUE * args = reinterpret_cast<VALUE *>(data);
        Sandbox * sandbox = reinterpret_cast<Sandbox *>(args[0]);
        rb_funcall(sandbox->GetSandbox(), rb_intern("load"), 1, args[1]);
        return Qnil;
	}
	
	/*#############################################################
	  ###########################################################*/

	Safe::Safe(float timeout)
	{
        VALUE options = rb_hash_new();

        if (timeout != 0)
        {
            rb_funcall(options, rb_intern("[]="), 2, ID2SYM(rb_intern("timeout")), rb_float_new(timeout));
        }
        
        VALUE args[] = { rb_cObject, rb_str_new2("Sandbox") };
		int status = 0;
		VALUE cSandbox = rb_protect(ConstGetProtect, reinterpret_cast<VALUE>(args), &status);
		CheckStatus(status);
        status = 0;
        VALUE args2[] = { cSandbox, rb_str_new2("Safe") };
        VALUE cSafe = rb_protect(ConstGetProtect, reinterpret_cast<VALUE>(args2), &status);
        CheckStatus(status);
        
		sandbox_ = rb_funcall(cSafe,
                              rb_intern("new"),
                              1,
                              options);
	}
	
	Safe::~Safe()
	{
	}
}

#endif // HAVE_LIBRUBY