#include "scripting/sScripting.h"

#ifdef ENABLE_SCRIPTING

tCallbackScripting::tCallbackScripting(tCallbackScripting *& anchor)
        :tListItem<tCallbackScripting>(anchor), block(tScripting::GetInstance().GetProcRef(""))
{
}

void tCallbackScripting::Exec(tCallbackScripting *anchor) {
    if (anchor) {
        tScripting::GetInstance().Exec(anchor->block, NULL);
        Exec(anchor->Next());
    }
}

#ifdef WITH_RUBY 
#ifdef MACOSX_XCODE
#   include "AARuby.h"
#endif
#endif

/*#############################################################
  # RUBY SPECIFIC
  ###########################################################*/

#ifdef WITH_RUBY

    void tScripting::CheckStatus(int status) {
        if (status)
        {
            std::runtime_error error(GetExceptionInfo());
            throw error;
        }
    }

    std::string tScripting::GetExceptionInfo()
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

    tScripting::data_type tScripting::ExecProtect(tScripting::data_type value) {
        return rb_funcall(value, rb_intern("call"), 0);
    }

    tScripting::data_type tScripting::LoadProtect(tScripting::data_type value) {
        VALUE * args = reinterpret_cast<VALUE *>(value);
        rb_funcall(rb_mKernel, rb_intern("load"), 1, args[0], args[1]);
        return Qnil;
    }

    tScripting::data_type tScripting::EvalProtect(tScripting::data_type value) {
        return rb_eval_string(reinterpret_cast<char *>(value));;
    }

    tScripting::tScripting() {
    }

extern "C" {
    extern void Init_armagetronad(void);
}

    void tScripting::InitializeInterpreter() {
        ruby_init();
        VALUE load_path = rb_gv_get("$LOAD_PATH");
        rb_ary_push(load_path, rb_str_new2("."));
        Init_armagetronad();
        ruby_script("Armagetron Advanced");
        try {
            GetInstance().Load(tDirectories::Data(), "scripts/initialize.rb");
        }
        catch (std::runtime_error & e) {
            std::cerr << e.what() << '\n';
        }
    }

    void tScripting::CleanupInterpreter() {
        ruby_finalize();
    }

    void tScripting::Load(const tPath & path, const char * filename, bool w) {
        VALUE wrap = w ? Qtrue : Qfalse;
        tString realPath = path.GetReadPath(filename);
        if (realPath == "")
            return;

        VALUE args[] = {rb_str_new2(realPath.c_str()), wrap};
        int status = 0;
        rb_protect(LoadProtect, reinterpret_cast<VALUE>(args), &status);
        CheckStatus(status);
    }

    tScripting::data_type tScripting::Eval(std::string code) {
        int status = 0;
        VALUE result = rb_protect(EvalProtect, reinterpret_cast<VALUE>(code.c_str()), &status);
        CheckStatus(status);
        return result;
    }

    tScripting::proc_type tScripting::GetProcRef(std::string code) {
        return rb_block_proc();
    }

    void tScripting::Exec(proc_type proc, args *data) {
        int status = 0;
        rb_protect(ExecProtect, proc, &status);
        CheckStatus(status);
    }

/*#############################################################
  # LUA SPECIFIC
  ###########################################################*/

#elif defined(WITH_LUA)

    tScripting::tScripting() {
    }

    void tScripting::InitializeInterpreter() {
    }

    void tScripting::CleanupInterpreter() {
    }

    void tScripting::Load(const tPath & path, const char * filename, bool w) {
    }

    tScripting::data_type tScripting::Eval(std::string code) {
    }

    tScripting::proc_type tScripting::GetProcRef(std::string code) {
    }

    void tScripting::Exec(proc_type proc, args *data) {
    }

/*#############################################################
  # PHP SPECIFIC
  ###########################################################*/

#elif defined(WITH_PHP)

    tScripting::tScripting() {
    }

    void tScripting::InitializeInterpreter() {
    }

    void tScripting::CleanupInterpreter() {
    }

    void tScripting::Load(const tPath & path, const char * filename, bool w) {
    }

    tScripting::data_type tScripting::Eval(std::string code) {
    }

    tScripting::proc_type tScripting::GetProcRef(std::string code) {
    }

    void tScripting::Exec(proc_type proc, args *data) {
    }

/*#############################################################
  # PYTHON SPECIFIC
  ###########################################################*/

#elif defined(WITH_PYTHON)

    tScripting::tScripting():
        main_module(0),main_dict(0) {
    }

extern "C" {
    extern void init_armagetronad(void);
}

    void tScripting::InitializeInterpreter() {
        Py_Initialize();
	// Get a reference to the main module.
	main_module = PyImport_AddModule("__main__");
	// Get the main module's dictionary
	main_dict = PyModule_GetDict(main_module);

	init_armagetronad();
        GetInstance().Load(tDirectories::Data(), "scripts/initialize.py");
    }

    void tScripting::CleanupInterpreter() {
        Py_XDECREF(main_dict); main_dict=0;
        Py_Finalize();
    }

    void tScripting::Load(const tPath & path, const char * filename, bool w) {
        tString realPath = path.GetReadPath(filename);
        if (realPath == "")
            return;

        tCurrentAccessLevel elevator( tAccessLevel_Owner, true );
        FILE* file = fopen(realPath.c_str(), "r");
        PyRun_File(file, filename,
           Py_file_input,
           main_dict, main_dict);
        fclose(file);
        if(PyErr_Occurred()) PyErr_Print();
    }

    tScripting::data_type tScripting::Eval(std::string code) {
        return NULL;
    }

    tScripting::proc_type tScripting::GetProcRef(std::string name) {
        PyObject *result;
        result = PyObject_GetAttrString(main_module, name.c_str());
        if (result && PyCallable_Check(result)) return result;
        if (PyErr_Occurred())
            PyErr_Print();
        con << "Cannot find function " << name << "\n";
        Py_XDECREF(result);
        return NULL;
    }

    void tScripting::Exec(proc_type proc, args *data) {
        if (!proc||!PyCallable_Check(proc)) return;

        PyObject *result, *params;
        tCurrentAccessLevel elevator( tAccessLevel_Owner, true );
        params = PyList_AsTuple(data->get());
        result = PyEval_CallObject(proc, params);
        if(PyErr_Occurred()) PyErr_Print();
        Py_XDECREF(params);
        Py_XDECREF(result);
    }

    args::args() {
        data = PyList_New(0);
        Py_INCREF(data);
    }

    args::~args() {
        Py_DECREF(data);
    }

    void args::clear() {
        Py_DECREF(data);
        data=PyList_New(0);
        Py_INCREF(data);
    }

    args &args::operator<< (args x){
        if (!data) return *this;
        PyList_Append(data, x.data);
        return *this;
    }

    args &args::operator<< (tScripting::data_type x){
        if (!data) return *this;
        PyList_Append(data, x);
        return *this;
    }

    args &args::operator<< (int x){
        if (!data) return *this;
        PyObject *item = Py_BuildValue("i", x);
        if (item) {
            PyList_Append(data, item);
            Py_DECREF(item);
        } else {}
        return *this;
    }

    args &args::operator<< (double x){
        if (!data) return *this;
        PyObject *item = Py_BuildValue("d", x);
        if (item) {
            PyList_Append(data, item);
            Py_DECREF(item);
        } else {}
        return *this;
    }

    args &args::operator<< (tOutput const &x){
        if (!data) return *this;
        tString ret;
        ret << x;
        PyObject *item = Py_BuildValue("s", (std::string(ret)).c_str());
        if (item) {
            PyList_Append(data, item);
            Py_DECREF(item);
        } else {}
        return *this;
    }

    args &args::operator<< (tString &x){
        if (!data) return *this;
        PyObject *item = Py_BuildValue("s", (std::string(x)).c_str());
        if (item) {
            PyList_Append(data, item);
            Py_DECREF(item);
        } else {}
        return *this;
    }

#endif

#endif // ENABLE_SCRIPTING
