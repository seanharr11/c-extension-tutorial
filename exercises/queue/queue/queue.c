#include <Python.h>

typedef struct {
    PyObject q_base;       /* storage for our type and reference count */
    Py_ssize_t q_maxsize;  /* the maximum number of elements in q_elements */
    PyObject* q_elements;  /* the elements in the queue as a Python list */
} queue;

static PyObject*
queue_new(PyTypeObject* cls, PyObject* args, PyObject* kwargs)
{
    static char* keywords[] = {"maxsize", NULL};

    queue* self;
    Py_ssize_t maxsize = -1;

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|n:Queue",
                                     keywords,
                                     &maxsize)) {
        /* argument parsing failed */
        return NULL;
    }

    /* Allocate memory for the instance with `tp_alloc`. We are not a varobject
       so `tp_itemsize` is 0 and we can pass 0 for `nitems`. */
    if (!(self = (queue*) cls->tp_alloc(cls, 0))) {
        /* allocation of the instance failed */
        return NULL;
    }

    /* Allocate a new empty list to hold the elements. This gives ownership of
       the list to `self`. */
    if (!(self->q_elements = PyList_New(0))) {
        /* allocation of the list failed */
        Py_DECREF(self);
        return NULL;
    }

    /* store the maxsize on the instance */
    self->q_maxsize = maxsize;

    /* erase the type queue c level type information and return to Python as a
       generic object */
    return (PyObject*) self;

}

static void
queue_dealloc(queue* self)
{
    /* tell the cyclic gc to stop watching our object */
    PyObject_GC_UnTrack(self);

    /* deallocate our elements list */
    Py_CLEAR(self->q_elements);

    /* deallocate our self */
    Py_TYPE(self)->tp_free(self);
}

static int
queue_traverse(queue* self, visitproc visit, void* arg)
{
    /* Visit our elements list. The list's visit function will visit all of
       the members. */
    Py_VISIT(self->q_elements);

    /* 0 means success */
    return 0;
}

static int
queue_clear(queue* self)
{
    /* call the equivalent of `del self->q_elements[:]` in Python to remove all
     * the members of our list which would clear any cycles */
    if (PyList_SetSlice(self->q_elements,
                        0,  /* start */
                        PyList_GET_SIZE(self->q_elements), /* stop */
                        NULL  /* sequence (NULL means delete */)) {
        /* nonzero means failure */
        return 1;
    }

    /* 0 means success */
    return 0;
}

static PyObject*
queue_repr(queue* self)
{
    if (self->q_maxsize < 0) {
        /* repr which looks like '<queue.Queue: 4>' with 4 elements in the
         * queue */
        return PyUnicode_FromFormat("<%s: %zd>",
                                    Py_TYPE(self)->tp_name,
                                    PyList_GET_SIZE(self->q_elements));
    }

    /* repr when we have a finite size which looks like: '<queue.Queue: 4/5>'
       with 4 elements in the queue and maxsize=5 */
    return PyUnicode_FromFormat("<%s: %zd/%zd>",
                                Py_TYPE(self)->tp_name,
                                PyList_GET_SIZE(self->q_elements),
                                self->q_maxsize);
}

static PyObject*
queue_push(queue* self, PyObject* args, PyObject* kwargs)
{
    PyErr_SetString(PyExc_NotImplementedError, "you need to implement push");
    return NULL;
}

static PyObject*
queue_pop(queue* self)
{
    PyErr_SetString(PyExc_NotImplementedError, "you need to implement pop");
    return NULL;
}

PyMethodDef queue_methods[] = {
    {"push", (PyCFunction) queue_push, METH_VARARGS | METH_KEYWORDS, NULL},
    {"pop", (PyCFunction) queue_pop, METH_NOARGS, NULL},
    {NULL},
};

/* PySequenceMethods layout for extra exercise */
PySequenceMethods queue_as_sequence = {
    0,                                          /* sq_length */
    0,                                          /* sq_concat */
    0,                                          /* sq_repeat */
    0,                                          /* sq_item */
    0,                                          /* placeholder */
    0,                                          /* sq_ass_item */
    0,                                          /* placeholder */
    0,                                          /* sq_contains */
    0,                                          /* sq_inplace_concat */
    0,                                          /* sq_inplace_repeat */
};

PyDoc_STRVAR(queue_doc, "A simple queue.");

static PyTypeObject queue_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "queue.Queue",                              /* tp_name */
    sizeof(queue),                              /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor) queue_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc) queue_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
    Py_TPFLAGS_HAVE_GC,                         /* tp_flags */
    queue_doc,                                  /* tp_doc */
    (traverseproc) queue_traverse,              /* tp_traverse */
    (inquiry) queue_clear,                      /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    queue_methods,                              /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    (newfunc) queue_new,                        /* tp_new */
};

PyModuleDef queue_module = {
    PyModuleDef_HEAD_INIT,
    "queue.queue",
    NULL,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit_queue(void) {
    PyObject* m;

    /* 'Ready' the type. This copies functions and data down from our subclass
       so that `queue_type` is in a valid state. */
    if (PyType_Ready(&queue_type)) {
        /* failed to ready the type */
        return NULL;
    }

    if (!(m = PyModule_Create(&queue_module))) {
        /* failed to allocate the module object */
        return NULL;
    }

    if (PyObject_SetAttrString(m, "Queue", (PyObject*) &queue_type)) {
        /* failed to store Queue on the module */
        Py_DECREF(m);
        return NULL;
    }

    return m;
}