#define PY_SSIZE_T_CLEAN
#include <Python.h>

/*  
############################################################################
    int type definitions
############################################################################
*/

#ifndef uint8
typedef unsigned char uint8;
#endif
#ifndef uint16
typedef unsigned short uint16;
#endif
#ifndef uint32
typedef unsigned int uint32;
#endif
#ifndef uint64
typedef unsigned long long uint64;
#endif

#ifndef int8
typedef signed char int8;
#endif
#ifndef int16
typedef signed short int16;
#endif
#ifndef int32
typedef signed int int32;
#endif
#ifndef int64
typedef signed long long int64;
#endif

/*  
############################################################################
    endianess definitions
############################################################################
*/
// check if the system is little endian
#define IS_LITTLE_ENDIAN (*(unsigned char *)&(uint16_t){1})

// set swap funcions (source: old version of nodejs/src/node_buffer.cc)
#if defined(__GNUC__) || defined(__clang__)
#define bswap16(x) __builtin_bswap16(x)
#define bswap32(x) __builtin_bswap32(x)
#define bswap64(x) __builtin_bswap64(x)
#elif defined(__linux__)
#include <byteswap.h>
#define bswap16(x) bswap_16(x)
#define bswap32(x) bswap_32(x)
#define bswap64(x) bswap_64(x)
#elif defined(_MSC_VER)
#include <intrin.h>
#define bswap16(x) _byteswap_ushort(x)
#define bswap32(x) _byteswap_ulong(x)
#define bswap64(x) _byteswap_uint64(x)
#else
#define bswap16 ((x) << 8) | ((x) >> 8)
#define bswap32                 \
    (((x)&0xFF) << 24) |        \
        (((x)&0xFF00) << 8) |   \
        (((x) >> 8) & 0xFF00) | \
        (((x) >> 24) & 0xFF)
#define bswap64                               \
    (((x)&0xFF00000000000000ull) >> 56) |     \
        (((x)&0x00FF000000000000ull) >> 40) | \
        (((x)&0x0000FF0000000000ull) >> 24) | \
        (((x)&0x000000FF00000000ull) >> 8) |  \
        (((x)&0x00000000FF000000ull) << 8) |  \
        (((x)&0x0000000000FF0000ull) << 24) | \
        (((x)&0x000000000000FF00ull) << 40) | \
        (((x)&0x00000000000000FFull) << 56)
#endif

/*  
############################################################################
    BinaryReader base class definition
############################################################################
*/
typedef struct
{
    PyObject_HEAD
        PyObject *obj;
    char *data;
    char *cur;
    char *end;
    Py_ssize_t size;
    char is_sys_endianess;
} BinaryReaderObject;

static int
BinaryReader_init(BinaryReaderObject *self, PyObject *args, PyObject *kwds)
{
    // parse the arguments
    PyObject *object;
    char is_little_endian = 0;
    if (!PyArg_ParseTuple(args, "O|b", &object, &is_little_endian))
    {
        return NULL;
    }

    // extract pointer to underlying buffer
    // bytearray == r/w buffer
    if (PyByteArray_CheckExact(object))
    {
        self->data = PyByteArray_AsString(object);
        self->size = PyByteArray_Size(object);
    }
    // bytes == r buffer
    else if (PyBytes_CheckExact(object))
    {
        self->data = PyBytes_AsString(object);
        self->size = PyBytes_Size(object);
    }
    // buffer interface = r buffer?
    else if (PyObject_CheckBuffer(object))
    {
        Py_buffer view;
        if (PyObject_GetBuffer(object, &view, PyBUF_SIMPLE) < 0)
        {
            return -1;
        }
        self->data = (char *)view.buf;
        self->size = view.len;
        PyBuffer_Release(&view);
    }
    // unsupported type
    else
    {
        PyErr_SetString(PyExc_TypeError, "Expected bytearray, bytes or buffer");
        return -1;
    }

    // assign values to BinaryReader instance
    self->obj = object;
    Py_INCREF(object);
    self->cur = self->data;
    self->end = self->data + self->size;
    self->is_sys_endianess = is_little_endian == IS_LITTLE_ENDIAN;
    return 0;
}

static void BinaryReader_dealloc(BinaryReaderObject *self)
{
    Py_XDECREF(self->obj);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

/*  
############################################################################
    BinaryReader property definitions
############################################################################
*/
static PyObject *
BinaryReader_getPosition(BinaryReaderObject *self, void *closure)
{
    return PyLong_FromUnsignedLongLong((uint64)(self->cur - self->data));
}

static int
BinaryReader_setPosition(BinaryReaderObject *self, PyObject *value, void *closure)
{
    if (value == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the position attribute");
        return -1;
    }
    if (!PyLong_Check(value))
    {
        PyErr_SetString(PyExc_TypeError,
                        "The position attribute value must be an int");
        return -1;
    }
    self->cur = self->data + PyLong_AsUnsignedLongLong(value);

    return 0;
}

static PyObject *
BinaryReader_getSize(BinaryReaderObject *self, void *closure)
{
    return PyLong_FromSsize_t(self->size);
}

static PyObject *
BinaryReader_getObj(BinaryReaderObject *self, void *closure)
{
    return self->obj;
}

static PyObject *
BinaryReader_getEndian(BinaryReaderObject *self, void *closure)
{
    return (self->is_sys_endianess == IS_LITTLE_ENDIAN) ? Py_True : Py_False;
}

static int
BinaryReader_setEndian(BinaryReaderObject *self, PyObject *value, void *closure)
{
    char is_little_endian = 0;
    if (!PyArg_ParseTuple(value, "b", &is_little_endian))
    {
        return 1;
    }
    self->is_sys_endianess = IS_LITTLE_ENDIAN == is_little_endian;
    return 0;
}

static PyGetSetDef BinaryReader_getsetters[] = {
    {"position", (getter)BinaryReader_getPosition, (setter)BinaryReader_setPosition,
     "the position of the cursor within the data", NULL},
    {"size", (getter)BinaryReader_getSize, NULL,
     "size of underlying/passed object", NULL},
    {"endian", (getter)BinaryReader_getEndian, (setter)BinaryReader_setEndian, "endianness of the reader (True - little, False - big)", NULL},
    {"obj", (getter)BinaryReader_getObj, NULL, "underlying/passed object", NULL},
    {NULL} /* Sentinel */
};

/*  
############################################################################
    helper functions
############################################################################
*/

/* macro function to generate a byte swap function*/
#define MAKE_ConvertEndian(S)                                                                   \
    inline static uint##S BinaryReader_convertEndian##S(BinaryReaderObject *self, uint##S data) \
    {                                                                                           \
        return self->is_sys_endianess ? data : bswap##S(data);                                  \
    }
/* apply macro function to generate a all required byte swap functions*/
MAKE_ConvertEndian(16); // (u)int16
MAKE_ConvertEndian(32); // (u)int32, half
MAKE_ConvertEndian(64); // (u)int64, double

/* check if the requested object can be read / is within the scope of the buffer */
inline static int BinaryReader_checkReadLength(BinaryReaderObject *self, Py_ssize_t length)
{
    if (self->cur + length > self->end)
    {
        PyErr_SetString(PyExc_ValueError, "read past end of buffer");
        return 1;
    }
    return 0;
}

/* parse the length of the buffer to be read and check if it can be read */
/* if a length is passed as argument, use it, otherwise read the length as int32*/
inline static int BinaryReader__readArrayLength(BinaryReaderObject *self, PyObject *args, char typeSize)
{
    int32 length = 0;
    if ((args) && (PyTuple_Size(args) == 1))
    {
        length = PyLong_AsLong(PyTuple_GetItem(args, 0));
    }
    else
    {
        BinaryReader_checkReadLength(self, 4);
        length = BinaryReader_convertEndian32(self, *(int32 *)self->cur);
        self->cur += 4;
    }
    if (BinaryReader_checkReadLength(self, length * typeSize))
    {
        return NULL;
    }
    return length;
}

/* internal function to cursor the stream to a given boundary */
inline void BinaryReader__alignC(BinaryReaderObject *self, char size)
{
    int padding = (size - ((self->cur - self->data) % size)) % size;
    self->cur += padding;
}

/* function to align the cursor to a given boundary */
static PyObject *
BinaryReader__align(BinaryReaderObject *self, PyObject *args)
{
    // TODO - parse args
    int size = 4;
    if (!PyArg_ParseTuple(args, "|i", &size))
    {
        return NULL;
    }
    return BinaryReader_getPosition(self, NULL);
}

/*  
############################################################################
    custom read functions (bool, (u)int8, half, string)
############################################################################
*/

static PyObject *
BinaryReader__readBool(BinaryReaderObject *self, PyObject *unused)
{
    BinaryReader_checkReadLength(self, 1);
    return *self->cur++ ? Py_True : Py_False;
}

static PyObject *
BinaryReader__readBoolArray(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 1);
    int8 *carray = self->cur;
    PyObject *pyarray = PyList_New(length);
    for (int i = 0; i < length; i++)
    {
        PyList_SET_ITEM(pyarray, i, carray[i] ? Py_True : Py_False);
    }
    self->cur += length;
    return pyarray;
}

static PyObject *
BinaryReader__readInt8(BinaryReaderObject *self, PyObject *unused)
{
    BinaryReader_checkReadLength(self, 1);
    return PyLong_FromLong((long)*(signed char *)self->cur++);
}

static PyObject *
BinaryReader__readInt8Array(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 1);
    int8 *carray = self->cur;
    PyObject *pyarray = PyList_New(length);
    for (int i = 0; i < length; i++)
    {
        PyList_SET_ITEM(pyarray, i, PyLong_FromLong((int32)carray[i]));
    }
    self->cur += length;
    return pyarray;
}

static PyObject *
BinaryReader__readUInt8(BinaryReaderObject *self, PyObject *unused)
{
    BinaryReader_checkReadLength(self, 1);
    return PyLong_FromLong((long)*(unsigned char *)self->cur++);
}

static PyObject *
BinaryReader__readUInt8Array(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 1);
    PyObject *pyarray = PyByteArray_FromStringAndSize(self->cur, length);
    self->cur += length;
    return pyarray;
}

static PyObject *
BinaryReader__readHalf(BinaryReaderObject *self, PyObject *unused)
{
    // borrowed from cpython/Modules/_struct.c
    BinaryReader_checkReadLength(self, 2);
    double x = _PyFloat_Unpack2(self->cur, self->is_sys_endianess == IS_LITTLE_ENDIAN);
    if (x == -1.0 && PyErr_Occurred())
    {
        return NULL;
    }
    self->cur += 2;
    return PyFloat_FromDouble(x);
}

static PyObject *
BinaryReader__readHalfArray(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 2);
    PyObject *pyarray = PyList_New(length);
    for (int i = 0; i < length; i++)
    {
        double x = _PyFloat_Unpack2(self->cur, self->is_sys_endianess == IS_LITTLE_ENDIAN);
        if (x == -1.0 && PyErr_Occurred())
        {
            return NULL;
        }
        self->cur += 2;
        PyList_SET_ITEM(pyarray, i, PyFloat_FromDouble(x));
    }
    return pyarray;
}

static PyObject *
BinaryReader__readStringNullTerminated(BinaryReaderObject *self, PyObject *unused)
{
    PyObject *string = PyUnicode_FromString(self->cur);
    self->cur += PyUnicode_GetLength(string) + 1; // +1 for null terminator
    return string;
}

static PyObject *
BinaryReader__readStringNullTerminatedArray(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 0);
    PyObject *pyarray = PyList_New(length);
    for (int i = 0; i < length; i++)
    {
        PyList_SET_ITEM(pyarray, i, BinaryReader__readStringNullTerminated(self, NULL));
    }
    return pyarray;
}

static PyObject *
BinaryReader__readStringLengthDelimited(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 1);
    PyObject *string = PyUnicode_FromStringAndSize(self->cur, length);
    self->cur += length;
    return string;
}

static PyObject *
BinaryReader__readStringLengthDelimitedArray(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 1);
    PyObject *pyarray = PyList_New(length);
    for (int i = 0; i < length; i++)
    {
        PyList_SET_ITEM(pyarray, i, BinaryReader__readStringLengthDelimited(self, NULL));
    }
    return pyarray;
}

static PyObject *
BinaryReader__readAlignedString(BinaryReaderObject *self, PyObject *args)
{
    PyObject *ret = BinaryReader__readStringLengthDelimited(self, args);
    BinaryReader__alignC(self, 4);
    return ret;
}

static PyObject *
BinaryReader__readAlignedStringArray(BinaryReaderObject *self, PyObject *args)
{
    int length = BinaryReader__readArrayLength(self, args, 1);
    PyObject *pyarray = PyList_New(length);
    for (int i = 0; i < length; i++)
    {
        PyList_SET_ITEM(pyarray, i, BinaryReader__readAlignedString(self, NULL));
    }
    return pyarray;
}

static PyObject *
BinaryReader__readVarInt(BinaryReaderObject *self, PyObject *unused)
{
    int64 value = 0;
    int shift = 0;
    uint8 byte;
    do
    {
        BinaryReader_checkReadLength(self, 1);
        byte = *self->cur++;
        value |= (byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    return PyLong_FromLongLong(value);
}

static PyObject *
BinaryReader__readLSB(BinaryReaderObject *self, PyObject *args)
{
    // if input is given, use it, otherwise use read all the data
    uint64 length = 0;
    if ((args) && (PyTuple_Size(args) == 1))
    {
        length = PyLong_AsLong(PyTuple_GetItem(args, 0));
        if (BinaryReader_checkReadLength(self, length))
        {
            return NULL;
        }
    }
    else
    {
        length = (uint64)(self->end - self->cur);
    }

    length = length / 8;

    unsigned char *data = (char *)PyMem_Malloc(length);
    unsigned char *cur = data;
    uint64 *raw = (uint64 *)self->cur;;
    if (self->is_sys_endianess == IS_LITTLE_ENDIAN)
    {
        // little endian, or big endian that has to be swapped
        // so we to check the highest bit of each byte within the uint64
        for (int i = 0; i < length; i++)
        {
            uint64 tmp = *raw++;
            *cur++ = (
                ((tmp >> 8*0)&1)<<7 | 
                ((tmp >> 8*1)&1)<<6 | 
                ((tmp >> 8*2)&1)<<5 | 
                ((tmp >> 8*3)&1)<<4 | 
                ((tmp >> 8*4)&1)<<3 | 
                ((tmp >> 8*5)&1)<<2 | 
                ((tmp >> 8*6)&1)<<1 | 
                ((tmp >> 8*7)&1)<<0
            );
        }
    }
    else
    {
        // big endian, or little endian that has to be swapped
        // so we to check the lowest bit of each byte within the uint64
        for (int i = 0; i < length; i++)
        {
            uint64 tmp = *raw++;
            *cur++ = (
                ((tmp >> 8*7)&1)<<7 | 
                ((tmp >> 8*6)&1)<<6 | 
                ((tmp >> 8*5)&1)<<5 | 
                ((tmp >> 8*4)&1)<<4 | 
                ((tmp >> 8*3)&1)<<3 | 
                ((tmp >> 8*2)&1)<<2 | 
                ((tmp >> 8*1)&1)<<1 | 
                ((tmp >> 8*0)&1)<<0
            );
        }
    }
    self->cur += length * 8;

    PyObject *pyarray = Py_BuildValue("y#", data, length);
    PyMem_Free(data);
    return pyarray;
}

/*  
############################################################################
    generic read functions ((u)int16, (u)int32, (u)int64, float, double)
############################################################################
*/

/* read element macro */
#define MAKE_READER(TYPE, TYPE_SIZE_BYTE, TYPE_SIZE_BIT, PYTHON_FUNC, PYTHON_FUNC_TYPE)                                \
    static PyObject *BinaryReader__read##TYPE(BinaryReaderObject *self, PyObject *args)                                \
    {                                                                                                                  \
        BinaryReader_checkReadLength(self, TYPE_SIZE_BYTE);                                                            \
        uint##TYPE_SIZE_BIT data = BinaryReader_convertEndian##TYPE_SIZE_BIT(self, *(uint##TYPE_SIZE_BIT *)self->cur); \
        self->cur += TYPE_SIZE_BYTE;                                                                                   \
        return PYTHON_FUNC((PYTHON_FUNC_TYPE) * (TYPE *)(&data));                                                      \
    }

/* read array macros */
#define MAKE_ARRAY_READER(TYPE, TYPE_SIZE_BYTE, TYPE_SIZE_BIT, PYTHON_FUNC, PYTHON_FUNC_TYPE)   \
    static PyObject *BinaryReader__read##TYPE##Array(BinaryReaderObject *self, PyObject *args)  \
    {                                                                                           \
        int length = BinaryReader__readArrayLength(self, args, TYPE_SIZE_BYTE);                 \
        PyObject *pyarray = PyList_New(length);                                                 \
        if (self->is_sys_endianess)                                                             \
        {                                                                                       \
            TYPE *carray = (TYPE *)self->cur;                                                   \
            for (int i = 0; i < length; i++)                                                    \
            {                                                                                   \
                PyList_SET_ITEM(pyarray, i, PYTHON_FUNC((PYTHON_FUNC_TYPE)carray[i]));          \
            }                                                                                   \
        }                                                                                       \
        else                                                                                    \
        {                                                                                       \
            uint##TYPE_SIZE_BIT *carray = (uint##TYPE_SIZE_BIT *)self->cur;                     \
            for (int i = 0; i < length; i++)                                                    \
            {                                                                                   \
                uint##TYPE_SIZE_BIT data = bswap##TYPE_SIZE_BIT(carray[i]);                     \
                PyList_SET_ITEM(pyarray, i, PYTHON_FUNC((PYTHON_FUNC_TYPE)(*(TYPE *)(&data)))); \
            }                                                                                   \
        }                                                                                       \
        self->cur += TYPE_SIZE_BYTE * length;                                                   \
        return pyarray;                                                                         \
    }

/* generate read element and read array functions macro */
#define MAKE_READER_FUNCS(TYPE, TYPE_SIZE_BYTE, TYPE_SIZE_BIT, PYTHON_FUNC, PYTHON_FUNC_TYPE) \
    MAKE_READER(TYPE, TYPE_SIZE_BYTE, TYPE_SIZE_BIT, PYTHON_FUNC, PYTHON_FUNC_TYPE)           \
    MAKE_ARRAY_READER(TYPE, TYPE_SIZE_BYTE, TYPE_SIZE_BIT, PYTHON_FUNC, PYTHON_FUNC_TYPE)

/* generate functions via macro */
MAKE_READER_FUNCS(int16, 2, 16, PyLong_FromLong, int32);
MAKE_READER_FUNCS(uint16, 2, 16, PyLong_FromUnsignedLong, uint32);
MAKE_READER_FUNCS(int32, 4, 32, PyLong_FromLong, int32);
MAKE_READER_FUNCS(uint32, 4, 32, PyLong_FromUnsignedLong, uint32);
MAKE_READER_FUNCS(int64, 8, 64, PyLong_FromLongLong, int64);
MAKE_READER_FUNCS(uint64, 8, 64, PyLong_FromUnsignedLongLong, uint64);
MAKE_READER_FUNCS(float, 4, 32, PyFloat_FromDouble, double);
MAKE_READER_FUNCS(double, 8, 64, PyFloat_FromDouble, double);

/*  
############################################################################
    add read functions to BinaryReaderObject as methods
############################################################################
*/

static PyMethodDef BinaryReader_methods[] = {
    {"readBool", (PyCFunction)BinaryReader__readBool, METH_NOARGS,
     PyDoc_STR("reads a bool")},
    {"readInt8", (PyCFunction)BinaryReader__readInt8, METH_NOARGS,
     PyDoc_STR("reads an int8")},
    {"readUInt8", (PyCFunction)BinaryReader__readUInt8, METH_NOARGS,
     PyDoc_STR("reads an uint8")},
    {"readInt16", (PyCFunction)BinaryReader__readint16, METH_NOARGS,
     PyDoc_STR("reads an int16")},
    {"readUInt16", (PyCFunction)BinaryReader__readuint16, METH_NOARGS,
     PyDoc_STR("reads an uint16")},
    {"readInt32", (PyCFunction)BinaryReader__readint32, METH_NOARGS,
     PyDoc_STR("reads an int32")},
    {"readUInt32", (PyCFunction)BinaryReader__readuint32, METH_NOARGS,
     PyDoc_STR("reads an uint32")},
    {"readInt64", (PyCFunction)BinaryReader__readint64, METH_NOARGS,
     PyDoc_STR("reads an int64")},
    {"readUInt64", (PyCFunction)BinaryReader__readuint64, METH_NOARGS,
     PyDoc_STR("reads an uint64")},
    {"readHalf", (PyCFunction)BinaryReader__readHalf, METH_NOARGS,
     PyDoc_STR("reads a half")},
    {"readFloat", (PyCFunction)BinaryReader__readfloat, METH_NOARGS,
     PyDoc_STR("reads a float")},
    {"readDouble", (PyCFunction)BinaryReader__readdouble, METH_NOARGS,
     PyDoc_STR("reads a double")},
    {"readBoolArray", (PyCFunction)BinaryReader__readBoolArray, METH_VARARGS,
     PyDoc_STR("reads a bool array")},
    {"readInt8Array", (PyCFunction)BinaryReader__readInt8Array, METH_VARARGS,
     PyDoc_STR("reads a array of int8")},
    {"readUInt8Array", (PyCFunction)BinaryReader__readUInt8Array, METH_VARARGS,
     PyDoc_STR("reads a array of uint8")},
    {"readInt16Array", (PyCFunction)BinaryReader__readint16Array, METH_VARARGS,
     PyDoc_STR("reads a array of int16")},
    {"readUInt16Array", (PyCFunction)BinaryReader__readuint16Array, METH_VARARGS,
     PyDoc_STR("reads a array of uint16")},
    {"readInt32Array", (PyCFunction)BinaryReader__readint32Array, METH_VARARGS,
     PyDoc_STR("reads a array of int32")},
    {"readUInt32Array", (PyCFunction)BinaryReader__readuint32Array, METH_VARARGS,
     PyDoc_STR("reads a array of uint32")},
    {"readInt64Array", (PyCFunction)BinaryReader__readint64Array, METH_VARARGS,
     PyDoc_STR("reads a array of int64")},
    {"readUInt64Array", (PyCFunction)BinaryReader__readuint64Array, METH_VARARGS,
     PyDoc_STR("reads a array of uint64")},
    {"readHalfArray", (PyCFunction)BinaryReader__readHalfArray, METH_VARARGS,
     PyDoc_STR("reads a array of half")},
    {"readFloatArray", (PyCFunction)BinaryReader__readfloatArray, METH_VARARGS,
     PyDoc_STR("reads a array of float")},
    {"readDoubleArray", (PyCFunction)BinaryReader__readdoubleArray, METH_VARARGS,
     PyDoc_STR("reads a array of double")},
    {"readStringC", (PyCFunction)BinaryReader__readStringNullTerminated, METH_VARARGS,
     PyDoc_STR("reads a null terminated string")},
    {"readStringCArray", (PyCFunction)BinaryReader__readStringNullTerminated, METH_VARARGS,
     PyDoc_STR("reads an array of null terminated strings")},
    {"readString", (PyCFunction)BinaryReader__readStringLengthDelimited, METH_VARARGS,
     PyDoc_STR("reads a string (if length is not passed as arg, read an int as length)")},
    {"readStringArray", (PyCFunction)BinaryReader__readStringLengthDelimitedArray, METH_VARARGS,
     PyDoc_STR("reads an array of strings")},
    {"readStringAligned", (PyCFunction)BinaryReader__readAlignedString, METH_VARARGS,
     PyDoc_STR("same as readString but aligned to 4 bytes after reading the string")},
    {"readStringAlignedArray", (PyCFunction)BinaryReader__readAlignedStringArray, METH_VARARGS,
     PyDoc_STR("reads an array of aligned strings")},
    {"align", (PyCFunction)BinaryReader__align, METH_VARARGS,
     PyDoc_STR("aligns the cursor to the given input")},
    {"readVarInt", (PyCFunction)BinaryReader__readVarInt, METH_NOARGS,
     PyDoc_STR("reads a varint")},
    {"readLSB", (PyCFunction)BinaryReader__readLSB, METH_VARARGS,
     PyDoc_STR("reads the lsb data of the given size (in bytes to read -> output length is 1/8 of that)")},
    {NULL},
};

/*  
############################################################################
    create BinaryReaderType and module for Python
############################################################################
*/

static PyTypeObject BinaryReaderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "binaryreader.BinaryReader",
    .tp_doc = "a BinaryReader that allows an easy and fast parsing of binary data",
    .tp_basicsize = sizeof(BinaryReaderObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = BinaryReader_methods,
    .tp_getset = BinaryReader_getsetters,
    .tp_init = (initproc)BinaryReader_init,
    .tp_dealloc = (destructor)BinaryReader_dealloc,
};

static PyModuleDef BinaryReadermodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "binaryreader",
    .m_doc = "a BinaryReader that allows an easy and fast parsing of binary data",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_binaryreader(void)
{
    PyObject *m;
    if (PyType_Ready(&BinaryReaderType) < 0)
        return NULL;

    m = PyModule_Create(&BinaryReadermodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&BinaryReaderType);
    if (PyModule_AddObject(m, "BinaryReader", (PyObject *)&BinaryReaderType) < 0)
    {
        Py_DECREF(&BinaryReaderType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}