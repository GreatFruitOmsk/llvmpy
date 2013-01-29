#include <Python.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Value.h>
#include <llvm/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace extra{
    using namespace llvm;
    
    class raw_svector_ostream_helper: public raw_svector_ostream {
        SmallVectorImpl<char> *SV;
    public:
        static
        raw_svector_ostream_helper* create()
        {
            SmallVectorImpl<char>* sv = new SmallVector<char, 16>();
            return new raw_svector_ostream_helper(sv);
        }

        ~raw_svector_ostream_helper()
        {
            delete SV;
        }

    protected:

        explicit
        raw_svector_ostream_helper(SmallVectorImpl<char>* sv)
        : raw_svector_ostream(*sv), SV(sv) {}

    private:
        // no copy
        raw_svector_ostream_helper(const raw_svector_ostream_helper&);
        // no assign
        void operator = (const raw_svector_ostream_helper&);
    };

}

static
PyObject* make_raw_ostream_for_printing(PyObject* self, PyObject* args)
{
    using extra::raw_svector_ostream_helper;
    using llvm::raw_svector_ostream;
    
    if (!PyArg_ParseTuple(args, "")) {
        return NULL;
    }
    raw_svector_ostream* RSOH = raw_svector_ostream_helper::create();
    return pycapsule_new(RSOH, "llvm::raw_ostream",
                         "llvm::raw_svector_ostream");
}

static
PyObject* make_small_vector_from_types(PyObject* self, PyObject* args)
{
    using llvm::Type;
    typedef llvm::SmallVector<llvm::Type*, 8> SmallVector_Type;
    
    SmallVector_Type* SV = new SmallVector_Type;
    Py_ssize_t size = PyTuple_Size(args);
    for (Py_ssize_t i = 0; i < size; ++i) {
        PyObject* cap = PyTuple_GetItem(args, i);
        if (!cap) {
            return NULL;
        }
        Type* type = (Type*)PyCapsule_GetPointer(cap, "llvm::Type");
        if (!type) {
            return NULL;
        }
        SV->push_back(type);
    }
    return pycapsule_new(SV, "llvm::SmallVector<llvm::Type*,8>");
}

static
PyObject* make_small_vector_from_values(PyObject* self, PyObject* args)
{
    using llvm::Value;
    typedef llvm::SmallVector<llvm::Value*, 8> SmallVector_Value;

    SmallVector_Value* SV = new SmallVector_Value;
    Py_ssize_t size = PyTuple_Size(args);
    for (Py_ssize_t i = 0; i < size; ++i) {
        PyObject* cap = PyTuple_GetItem(args, i);
        if (!cap) {
            return NULL;
        }
        Value* value = (Value*)PyCapsule_GetPointer(cap, "llvm::Value");
        if (!value) {
            return NULL;
        }
        SV->push_back(value);
    }
    return pycapsule_new(SV, "llvm::SmallVector<llvm::Value*,8>");
}


static
PyObject* make_small_vector_from_unsigned(PyObject* self, PyObject* args)
{
    using llvm::Value;
    typedef llvm::SmallVector<unsigned, 8> SmallVector_Unsigned;

    SmallVector_Unsigned* SV = new SmallVector_Unsigned;
    Py_ssize_t size = PyTuple_Size(args);
    for (Py_ssize_t i = 0; i < size; ++i) {
        PyObject* item = PyTuple_GetItem(args, i);
        if (!item) {
            return NULL;
        }
        unsigned value = PyLong_AsUnsignedLong(item);
        if (PyErr_Occurred()){
            return NULL;
        }
        SV->push_back(value);
    }
    return pycapsule_new(SV, "llvm::SmallVector<unsigned,8>");
}

static PyMethodDef extra_methodtable[] = {
    #define method(func) { #func, (PyCFunction)func, METH_VARARGS, NULL }
    method( make_raw_ostream_for_printing ),
    method( make_small_vector_from_types ),
    method( make_small_vector_from_values ),
    method( make_small_vector_from_unsigned ),
    #undef method
    { NULL }
};


////////////
template<class iterator>
PyObject* iterator_to_pylist_deref(iterator begin, iterator end,
                             const char *capsuleName, const char *className)
{
    PyObject* list = PyList_New(0);
    for(; begin != end; ++begin) {
        PyObject* cap = pycapsule_new(&*begin, capsuleName, className);
        PyList_Append(list, cap);
    }
    return list;
}

template<class iterator>
PyObject* iterator_to_pylist(iterator begin, iterator end,
                             const char *capsuleName, const char *className)
{
    PyObject* list = PyList_New(0);
    for(; begin != end; ++begin) {
        PyObject* cap = pycapsule_new(*begin, capsuleName, className);
        PyList_Append(list, cap);
    }
    return list;
}

template<class iplist>
PyObject* iplist_to_pylist(iplist &IPL, const char * capsuleName,
                           const char* className){
    return iterator_to_pylist_deref(IPL.begin(), IPL.end(), capsuleName,
                                    className);
}

static
bool string_equal(const char *A, const char *B){
    for (; *A and *B; ++A, ++B) {
        if (*A != *B) return false;
    }
    return true;
}

////////////
static
PyObject* Value_use_iterator_to_list(llvm::Value* val)
{
    return iterator_to_pylist(val->use_begin(), val->use_end(),
                              "llvm::Value", "llvm::User");
}

static
PyObject* Function_getArgumentList(llvm::Function* fn)
{
    return iplist_to_pylist(fn->getArgumentList(), "llvm::Value",
                            "llvm::Argument");
}

static
PyObject* Function_getBasicBlockList(llvm::Function* fn)
{
    return iplist_to_pylist(fn->getBasicBlockList(), "llvm::Value",
                            "llvm::BasicBlock");
}

/*
 * errout --- can be any file object
 *
 */
static
llvm::ExecutionEngine* ExecutionEngine_create(
                  llvm::Module* M,
                  bool ForceInterpreter = false,
                  PyObject* errout = 0,
                  llvm::CodeGenOpt::Level OptLevel = llvm::CodeGenOpt::Default,
                  bool 	GVsWithCode = true)
{
    using namespace llvm;
    std::string ErrorStr;
    ExecutionEngine *ee = ExecutionEngine::create(M, ForceInterpreter,
                                                  &ErrorStr, OptLevel,
                                                  GVsWithCode);
    PyFile_WriteString(ErrorStr.c_str(), errout);
    return ee;
}

/*
 * errout --- can be any file object
 *
 */
static
llvm::ExecutionEngine* ExecutionEngine_createJIT(
                     llvm::Module* M,
                     PyObject* errout = 0,
                     llvm::JITMemoryManager* JMM = 0,
                     llvm::CodeGenOpt::Level OL = llvm::CodeGenOpt::Default,
                     bool GCsWithCode = true,
                     llvm::Reloc::Model RM = llvm::Reloc::Default,
                     llvm::CodeModel::Model CMM = llvm::CodeModel::JITDefault)
{
    using namespace llvm;
    std::string ErrorStr;
    ExecutionEngine *ee = ExecutionEngine::createJIT(M, &ErrorStr, JMM, OL,
                                                     GCsWithCode, RM, CMM);
    PyFile_WriteString(ErrorStr.c_str(), errout);
    return ee;
}
