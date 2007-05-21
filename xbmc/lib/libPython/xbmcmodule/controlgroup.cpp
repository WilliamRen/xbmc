#include "stdafx.h"
#include "../python/Python.h"
#include "GUIControlGroup.h"
#include "GUIFontManager.h"
#include "control.h"
#include "pyutil.h"

#pragma code_seg("PY_TEXT")
#pragma data_seg("PY_DATA")
#pragma bss_seg("PY_BSS")
#pragma const_seg("PY_RDATA")

#ifdef __cplusplus
extern "C"
{
#endif
  namespace PYXBMC
  {
    PyObject * ControlGroup_New (PyTypeObject *type, PyObject *args, PyObject *kwds)
    {
      static char *keywords[] = {
        "x", "y", "width", "height", NULL
      };

      ControlGroup *self;
      int ret;

      self = (ControlGroup *) type->tp_alloc (type, 0);
      if (!self) {
        return NULL;
      }

      ret = PyArg_ParseTupleAndKeywords (args, kwds, "llll", keywords,
                                         &self->dwPosX, &self->dwPosY,
                                         &self->dwWidth, &self->dwHeight);
      if (!ret) {
        Py_DECREF (self);
        return NULL;
      }

      return (PyObject *) self;
    }


    void ControlGroup_Dealloc (ControlGroup *self)
    {
      self->ob_type->tp_free ((PyObject *) self);
    }


    CGUIControl *
    ControlGroup_Create (ControlGroup *pCtrl)
    {
      pCtrl->pGUIControl = new CGUIControlGroup(pCtrl->iParentId,
                                                pCtrl->iControlId,
                                                (float) pCtrl->dwPosX,
                                                (float) pCtrl->dwPosY,
                                                (float) pCtrl->dwWidth,
                                                (float) pCtrl->dwHeight);
      return pCtrl->pGUIControl;
    }


    PyMethodDef ControlGroup_methods[] = {
      {NULL, NULL, 0, NULL}
    };


    // ControlGroup class
    PyDoc_STRVAR (controlGroup__doc__,
        "ControlGroup class.\n"
        "\n"
        "ControlGroup(x, y, width, height\n"
        "\n"
        "x              : integer - x coordinate of control.\n"
        "y              : integer - y coordinate of control.\n"
        "width          : integer - width of control.\n"
        "height         : integer - height of control.\n"
        "example:\n"
        "  - self.group = xbmcgui.ControlGroup(100, 250, 125, 75)\n");

// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

    PyTypeObject ControlGroup_Type;

    void initControlGroup_Type ()
    {
      PyInitializeTypeObject (&ControlGroup_Type);

      ControlGroup_Type.tp_name = "xbmcgui.ControlGroup";
      ControlGroup_Type.tp_basicsize = sizeof (ControlGroup);
      ControlGroup_Type.tp_dealloc = (destructor) ControlGroup_Dealloc;
      ControlGroup_Type.tp_compare = 0;
      ControlGroup_Type.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
      ControlGroup_Type.tp_doc = controlGroup__doc__;
      ControlGroup_Type.tp_methods = ControlGroup_methods;
      ControlGroup_Type.tp_base = &Control_Type;
      ControlGroup_Type.tp_new = ControlGroup_New;
    }
  }
#ifdef __cplusplus
}
#endif
