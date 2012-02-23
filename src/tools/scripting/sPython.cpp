/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************************

*/

#include "scripting/sScripting.h"

#ifdef ENABLE_SCRIPTING

#ifdef WITH_PYTHON

extern "C" {
#include <Python.h>
}

class sDataPython: public sData {
    PyObject *m_data;
public:
    sDataPython():sData(),m_data(0) {}
    sDataPython(const sDataPython & p_source):sData(),m_data(0) { Copy(p_source); }
    virtual ~sDataPython() {
        Clear();
    }

// clear current data content
    virtual void                Clear();

// getters
    virtual int                 GetInt() const;
    virtual REAL                GetReal() const;
    virtual const char *        GetString() const;

// setters
    virtual void Set(const int &i);
    virtual void Set(const REAL &r);
    virtual void Set(const char* s);

// checkers
    virtual const bool IsInt();
    virtual const bool IsReal();
    virtual const bool IsString();

// copy and assignment
    virtual void Copy(const sData & source);

// comparisons (required to build args list)
    virtual bool Equal(sData const &a) const;

// ugly get/set on raw PyObject *
// these assumed you'll handled ref counting on your own for the pointers you get/set
    PyObject * Get() const { return m_data; }
    void Set(PyObject * obj) { Py_XDECREF(m_data); m_data = obj; }
};

// clear content
void sDataPython::Clear() {
    // release existing data if needed
    Py_XDECREF(m_data);
    m_data = nullptr;
}

// getters
int sDataPython::GetInt() const {
    return (m_data ? PyInt_AS_LONG(m_data) : 0);
}

REAL sDataPython::GetReal() const {
    return (m_data ? PyFloat_AS_DOUBLE(m_data) : .0);
}

const char * sDataPython::GetString() const {
    return (m_data ? PyString_AsString(m_data) : "");
}
    
// setters
void sDataPython::Set(const int &i) {
    // release existing data if needed
    Py_XDECREF(m_data);
    // set the new value
    m_data = PyInt_FromLong(i);
}

void sDataPython::Set(const REAL &r) {
    // release existing data if needed
    Py_XDECREF(m_data);
    // set the new value
    m_data = PyFloat_FromDouble(r);
}

void sDataPython::Set(const char* s) {
    // release existing data if needed
    Py_XDECREF(m_data);
    // set the new value
    if (s) {
        m_data = PyString_FromString(s);
    } else {
        m_data = nullptr;
    }
}

// checkers
const bool sDataPython::IsInt() {
    return (!m_data && PyInt_Check(m_data));
}

const bool sDataPython::IsReal() {
    return (!m_data && PyFloat_Check(m_data));
}

const bool sDataPython::IsString() {
    return (!m_data && PyString_Check(m_data));
}

// copy and assignment
void sDataPython::Copy(const sData & source) {
    // release existing data if needed
    Py_XDECREF(m_data);
    // retain source
    const sDataPython * py_source = dynamic_cast<const sDataPython *>(&source);
    if (py_source && py_source->m_data) {
        m_data = py_source->m_data;
        Py_INCREF(m_data);
    } else {
        m_data = nullptr;
    }
}

// comparisons (required to build args list)
// in term of PyObject, not content
bool sDataPython::Equal(sData const &a) const {
    const sDataPython * py_a = dynamic_cast<const sDataPython *>(&a);
    if (py_a) return m_data == py_a->m_data;
    return false;
}


class sArgsPython: public sArgs {
    PyObject* m_data;
public:
    sArgsPython():sArgs(), m_data(0) {
    }
    virtual ~sArgsPython() {
        Py_XDECREF(m_data);
    }

    PyObject* ExistsOrCreate();    //<! Check if the list exists and try to create it if needed

    virtual void Clear();          //<! Clear the argument list

    // add parameters for a future call
    virtual sArgs & operator<< (int const & i);
    virtual sArgs & operator<< (REAL const & r);
    virtual sArgs & operator<< (tOutput const & x);
    virtual sArgs & operator<< (tString const & x);
    virtual sArgs & operator<< (sData const & d);

// ugly get/set on raw PyObject *
    PyObject * Get() const { return m_data; }
    void Set(const sArgsPython & p_args) {
        Py_XDECREF(m_data);
        m_data = p_args.Get();
        if (m_data) Py_INCREF(m_data);
    }
};

PyObject* sArgsPython::ExistsOrCreate() {
    // does the args list exist ? If not, let's try to create it...
    if (!m_data) {
        m_data=PyList_New(0);
        if(PyErr_Occurred()) PyErr_Print();
    }
    return m_data;
}

void sArgsPython::Clear() {
    Py_XDECREF(m_data);
    m_data=0;
}

sArgs & sArgsPython::operator<< (int const & i) {
    // does the list exist ? If not, let's try to create it. If it fails, return ...
    if (!ExistsOrCreate()) {
        return *this;
    }
    sDataPython item; // no need to DECREF as sDataPython is RAII
    item.Set(i);
    if (item.Get()) {
        PyList_Append(m_data, item.Get());
    }
    return *this;
}

sArgs & sArgsPython::operator<< (REAL const & r) {
    // does the list exist ? If not, let's try to create it. If it fails, return ...
    if (!ExistsOrCreate()) {
        return *this;
    }
    sDataPython item; // no need to DECREF as sDataPython is RAII
    item.Set(r);
    if (item.Get()) {
        PyList_Append(m_data, item.Get());
    }
    return *this;
}

sArgs & sArgsPython::operator<< (tOutput const & x) {
    // does the list exist ? If not, let's try to create it. If it fails, return ...
    if (!ExistsOrCreate()) {
        return *this;
    }
    sDataPython item; // no need to DECREF as sDataPython is RAII
    item.Set(x);
    if (item.Get()) {
        PyList_Append(m_data, item.Get());
    }
    return *this;
}

sArgs & sArgsPython::operator<< (tString const & x) {
    // does the list exist ? If not, let's try to create it. If it fails, return ...
    if (!ExistsOrCreate()) {
        return *this;
    }
    sDataPython item; // no need to DECREF as sDataPython is RAII
    item.Set(x);
    if (item.Get()) {
        PyList_Append(m_data, item.Get());
    }
    return *this;
}

sArgs & sArgsPython::operator<< (sData const & d) {
    const sDataPython * py_source = dynamic_cast<const sDataPython *>(&d);
    if (!py_source || !py_source->Get()) {
        //TODO: Handle error here, too.
        return *this;
    }

    // does the list exist ? If not, let's try to create it. If it fails, return ...
    if (!ExistsOrCreate()) {
        return *this;
    }
    PyList_Append(m_data, py_source->Get());
    return *this;
}


class sCallablePython: public sCallable {
    PyObject * m_callable;
    sArgsPython m_args;
public:
    sCallablePython():m_callable(0) {}
    sCallablePython(PyObject *p_proc):m_callable(0) { if (PyCallable_Check(p_proc)) m_callable = p_proc; }

    virtual ~sCallablePython() {};

    // add parameters for a future call
    virtual sCallable & operator<< (int const & i);
    virtual sCallable & operator<< (REAL const & r);
    virtual sCallable & operator<< (tOutput const & x);
    virtual sCallable & operator<< (tString const & x);
    virtual sCallable & operator<< (sData const & d);

    virtual sCallable & operator<< (sArgs const & d);

    virtual void Set (const char * funcname); //<! Set this callable to script function called
    virtual void Set (const sData & object);  //<! Set this callable to this script object
    virtual void Call();                      //<! Execute
    virtual void Clear();                     //<! Clear the argument list
};

sCallable & sCallablePython::operator<< (int const & i) {
    m_args << i;
    return *this;
}

sCallable & sCallablePython::operator<< (REAL const & r) {
    m_args << r;
    return *this;
}

sCallable & sCallablePython::operator<< (tOutput const & x) {
    m_args << x;
    return *this;
}

sCallable & sCallablePython::operator<< (tString const & x) {
    m_args << x;
    return *this;
}

sCallable & sCallablePython::operator<< (sData const & d) {
    m_args << d;
    return *this;
}

sCallable & sCallablePython::operator<< (sArgs const & d) {
    const sArgsPython * py_d = dynamic_cast<const sArgsPython *>(&d);
    // if provided args is not python args or is empty,
    // just clear current callable args and return
    if ((!py_d) || (!py_d->Get())) {
        m_args.Clear();
        return *this;
    }
    // otherwise, replace current callable args
    m_args.Set(*py_d);
    return *this;
}

void sCallablePython::Set (const char * funcname) {
}

void sCallablePython::Set (const sData & object) {
}

void sCallablePython::Call() {
    if (!m_callable) return;

    sDataPython params, result;
    tCurrentAccessLevel elevator( tAccessLevel_Owner, true );
    // Handle properly the case where args is empty.
    if (m_args.Get()) {
        params.Set(PyList_AsTuple(m_args.Get()));
    } else {
        params.Set(PyTuple_New(0));
    }
        result.Set(PyEval_CallObject(m_callable, params.Get()));
    if(PyErr_Occurred()) PyErr_Print();
}

void sCallablePython::Clear() {
}

class sScriptingPython: public sScripting {
    PyObject *main_module, *main_dict;
public:
    sScriptingPython();
    virtual ~sScriptingPython() {};

    // Init scripting
    virtual void InitializeInterpreter();
    // Stop (and clean) scripting
    virtual void CleanupInterpreter();

//    virtual sCallable::ptr Create_Callable();
//    virtual sArgs::ptr CreateArgs() const;

    // load (and exec) a file
    virtual void Load(const tPath & p_path, const char * p_filename, bool p_w=false);
    // Eval (and exec) a string
    virtual sData::ptr Eval(const std::string & p_code) const;

    // get a internal reference to an already defined proc (for callbacks)
    // if proc is empty, check for an unnamed function or a block
    virtual sCallable::ptr GetProcRef(const std::string & p_proc) const;

    // virtual constructor idiom
    virtual sData::ptr CreateData() const;
    virtual sArgs::ptr CreateArgs() const;
};

sScripting * sScripting::GetInstance() {
    static boost::shared_ptr<sScripting> _instance(new sScriptingPython());
    return _instance.get();
}

sScriptingPython::sScriptingPython():
    sScripting(), main_module(nullptr), main_dict(nullptr) {
}

extern "C" {
    extern void init_armagetronad(void);
}

void sScriptingPython::InitializeInterpreter() {
    Py_Initialize();
    // Get a reference to the main module.
    main_module = PyImport_AddModule("__main__");
    // Get the main module's dictionary
    main_dict = PyModule_GetDict(main_module);

    init_armagetronad();
    GetInstance()->Load(tDirectories::Data(), "scripts/initialize.py");
}

void sScriptingPython::CleanupInterpreter() {
    Py_XDECREF(main_dict); main_dict=0;
    Py_Finalize();
}

void sScriptingPython::Load(const tPath & path, const char * filename, bool w) {
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

sData::ptr sScriptingPython::Eval(const std::string & p_code) const {
    return sData::ptr();
}

sCallable::ptr sScriptingPython::GetProcRef(const std::string & p_proc) const {
    PyObject *result;
    result = PyObject_GetAttrString(main_module, p_proc.c_str());
    if (result && PyCallable_Check(result))
        return boost::shared_ptr<sCallable>(new sCallablePython(result));
    if (PyErr_Occurred())
        PyErr_Print();
    con << "Cannot find function " << p_proc << "\n";
    Py_XDECREF(result);
    return sCallable::ptr();
}

// virtual constructor idiom
sData::ptr sScriptingPython::CreateData() const
{
    return sData::ptr(new sDataPython);
}

sArgs::ptr sScriptingPython::CreateArgs() const
{
    return sArgs::ptr(new sArgsPython);
}

sCallable::ptr CreateCallablePython(PyObject * input) {
    return boost::shared_ptr<sCallable>(new sCallablePython(input));
}

#endif

#endif // ENABLE_SCRIPTING
