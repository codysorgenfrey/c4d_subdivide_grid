#ifndef SUBDIVIDEGRID_H__
#define SUBDIVIDEGRID_H__

#include "main.h"
#include "sortedarray.h"

#define ID_SUBDIVIDEGRID 1054125

enum {
  ID_SG_COMPLETE = 1000,
  ID_SG_SPLINE_GROUP = 1001,
  ID_SG_SPLINE_X = 1002,
  ID_SG_SPLINE_Y = 1003,
  ID_SG_SPLINE_Z = 1004,
  ID_SG_LIST = 1005,
};

namespace maxon
{

struct VectorBool
{
  Bool x, y, z;
  VectorBool()
  {
    x = false;
    y = false;
    z = false;
  }
  inline String Printable() {
    return("{"_s +
           " x:"_s + (x ? "true"_s : "false"_s) +
           " y:"_s + (y ? "true"_s : "false"_s) +
           " z:"_s + (z ? "true"_s : "false"_s) +
           " }"_s);
  };
};

struct SortedBBoxCorners : public SortedArray<SortedBBoxCorners, BaseArray<Vector>>
{
  static Vector anchor;
  static inline Bool LessThan(const Vector& a, const Vector& b) { return (anchor - a).GetLength() < (anchor - b).GetLength(); }
  static inline Bool IsEqual(const Vector& a, const Vector& b) { return (anchor - a).GetLength() == (anchor - b).GetLength(); }
};

Vector SortedBBoxCorners::anchor = { Vector(0) };

class SubdivideGrid : public TagData {
private:
  Bool GetBBox(BaseObject *object, BaseArray<Vector> &bbox);
  Vector GetRadFromBBox(BaseArray<Vector> &bbox);
  Bool GetCollectiveBBox(BaseArray<BaseObject *> &objects, BaseArray<Vector> &bbox);
  Bool GetCornersFromBBox(BaseArray<Vector> &bbox, WritableArrayInterface<Vector> &corners);
  VectorBool MakesFarSides(BaseArray<Vector> &bbox, Vector *farCorner);
  Vector GetPosInBBox(Vector pos, BaseArray<Vector> &bbox);
public:
  virtual Bool Init(GeListNode* node);
  virtual Bool GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC& flags);
  virtual Bool Message(GeListNode *node, Int32 type, void *data);
  virtual EXECUTIONRESULT Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags);
  static NodeData* Alloc() { return NewObjClear(SubdivideGrid); }
};
}

#endif
