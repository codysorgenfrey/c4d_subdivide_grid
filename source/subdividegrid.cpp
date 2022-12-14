#include "subdividegrid.h"
#include "c4d_symbols.h"
#include "customgui_splinecontrol.h"
#include "customgui_inexclude.h"

using namespace maxon;

//
// MARK: Helper Functions
//

inline Float MapRange(Float value, Float min_input, Float max_input, Float min_output, Float max_output, SplineData *curve = NULL)
{
  Float inrange = max_input - min_input;
  if (CompareFloatTolerant(SafeConvert<Float64>(inrange), SafeConvert<Float64>(0.0f)))
    value = 0.0f; // Prevent DivByZero error
  else
    value = (value - min_input) / inrange; // Map input range to [0.0 ... 1.0]
  
  if (curve)
    value = curve->GetPoint(value).y; // Apply spline curve
  
  return  min_output + (max_output - min_output) * value; // Map to output range and return result
}

inline Vector MinVector(Vector a, Vector b)
{
  Vector newVec;
  newVec.x = maxon::Min(a.x, b.x);
  newVec.y = maxon::Min(a.y, b.y);
  newVec.z = maxon::Min(a.z, b.z);
  return newVec;
}

inline Vector MaxVector(Vector a, Vector b)
{
  Vector newVec;
  newVec.x = maxon::Max(a.x, b.x);
  newVec.y = maxon::Max(a.y, b.y);
  newVec.z = maxon::Max(a.z, b.z);
  return newVec;
}

inline Vector roundOffVector(Vector in)
{
  Vector out;
  Float per = 10000;
  out.x = maxon::Floor(in.x * per + 0.5) / per;
  out.y = maxon::Floor(in.y * per + 0.5) / per;
  out.z = maxon::Floor(in.z * per + 0.5) / per;
  return out;
}

//
// MARK: Classes
//

Bool SubdivideGrid::Init(GeListNode* node)
{
  BaseTag *tag = (BaseTag *)node;
  BaseContainer *bc = tag->GetDataInstance();
  
  // complete control
  bc->SetFloat(ID_SG_COMPLETE, 1.0f);
  
  // list control
  GeData listData(CUSTOMDATATYPE_INEXCLUDE_LIST, DEFAULTVALUE);
  bc->SetData(ID_SG_LIST, listData);

  // spline x
  GeData splineDataX(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);
  SplineData *sx = (SplineData *)splineDataX.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
  if (sx)
    sx->MakeLinearSplineBezier(2);
  bc->SetData(ID_SG_SPLINE_X, splineDataX);
  
  // spline y
  GeData splineDataY(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);
  SplineData *sy = (SplineData *)splineDataY.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
  if (sy)
    sy->MakeLinearSplineBezier(2);
  bc->SetData(ID_SG_SPLINE_Y, splineDataY);
  
  // spline z
  GeData splineDataZ(CUSTOMDATATYPE_SPLINE, DEFAULTVALUE);
  SplineData *sz = (SplineData *)splineDataZ.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
  if (sz)
    sz->MakeLinearSplineBezier(2);
  bc->SetData(ID_SG_SPLINE_Z, splineDataZ);
  
  return true;
}

Bool SubdivideGrid::GetDDescription(GeListNode* node, Description* description, DESCFLAGS_DESC& flags)
{
  // Before adding dynamic parameters, load the parameters from the description resource
  if (!description->LoadDescription(node->GetType()))
    return false;

  // Get description single ID
  const DescID *singleID = description->GetSingleDescID();

  // Add Complete control
  DescID completeID = DescLevel(ID_SG_COMPLETE, DTYPE_REAL, 0);
  if (!singleID || completeID.IsPartOf(*singleID, nullptr))
  {
    BaseContainer bc = GetCustomDataTypeDefault(DTYPE_REAL);
    bc.SetString(DESC_NAME, "Complete"_s);
    bc.SetString(DESC_SHORT_NAME, "Complete"_s);
    bc.SetInt32(DESC_UNIT, DESC_UNIT_PERCENT);
    bc.SetInt32(DESC_CUSTOMGUI, CUSTOMGUI_REALSLIDER);
    bc.SetFloat(DESC_DEFAULT, 1.0f);
    bc.SetFloat(DESC_MIN, 0.0f);
    bc.SetFloat(DESC_MAX, 1.0f);
    bc.SetFloat(DESC_MINSLIDER, 0.0f);
    bc.SetFloat(DESC_MAXSLIDER, 1.0f);
    bc.SetFloat(DESC_STEP, 0.01f);
    bc.SetBool(DESC_GUIOPEN, true);
    if (!description->SetParameter(completeID, bc, ID_TAGPROPERTIES))
      return false;
  }
  
  // Add list control
  DescID listID = DescLevel(ID_SG_LIST, CUSTOMDATATYPE_INEXCLUDE_LIST, 0);
  if (!singleID || listID.IsPartOf(*singleID, nullptr))
  {
    BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_INEXCLUDE_LIST);
    bc.SetString(DESC_NAME, "Splines"_s);
    bc.SetString(DESC_SHORT_NAME, "Splines"_s);
    bc.SetBool(DESC_GUIOPEN, true);
    if (!description->SetParameter(listID, bc, ID_TAGPROPERTIES))
      return false;
  }

  // Add spline group
  DescID splineGroupID = DescLevel(ID_SG_SPLINE_GROUP, DTYPE_GROUP, 0);
  if (!singleID || splineGroupID.IsPartOf(*singleID, nullptr))
  {
    BaseContainer bc = GetCustomDataTypeDefault(DTYPE_GROUP);
    bc.SetString(DESC_NAME, "Time Ramps"_s);
    bc.SetString(DESC_SHORT_NAME, "Time Ramps"_s);
    bc.SetBool(DESC_GUIOPEN, false);
    if (!description->SetParameter(splineGroupID, bc, DESCID_ROOT))
      return false;
  }
  
  // Add x ramp control
  DescID splineXID = DescLevel(ID_SG_SPLINE_X, CUSTOMDATATYPE_SPLINE, 0);
  if (!singleID || splineXID.IsPartOf(*singleID, nullptr))
  {
    BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_SPLINE);
    bc.SetString(DESC_NAME, "X Ramp"_s);
    bc.SetString(DESC_SHORT_NAME, "X Ramp"_s);
    bc.SetBool(DESC_GUIOPEN, false);
    bc.SetFloat(SPLINECONTROL_X_MIN, 0.0f);
    bc.SetFloat(SPLINECONTROL_X_MAX, 1.0f);
    bc.SetFloat(SPLINECONTROL_Y_MIN, 0.0f);
    bc.SetFloat(SPLINECONTROL_Y_MAX, 1.0f);
    if (!description->SetParameter(splineXID, bc, splineGroupID))
      return false;
  }
  
  // Add y ramp control
  DescID splineYID = DescLevel(ID_SG_SPLINE_Y, CUSTOMDATATYPE_SPLINE, 0);
  if (!singleID || splineYID.IsPartOf(*singleID, nullptr))
  {
    BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_SPLINE);
    bc.SetString(DESC_NAME, "Y Ramp"_s);
    bc.SetString(DESC_SHORT_NAME, "Y Ramp"_s);
    bc.SetBool(DESC_GUIOPEN, false);
    bc.SetFloat(SPLINECONTROL_X_MIN, 0.0f);
    bc.SetFloat(SPLINECONTROL_X_MAX, 1.0f);
    bc.SetFloat(SPLINECONTROL_Y_MIN, 0.0f);
    bc.SetFloat(SPLINECONTROL_Y_MAX, 1.0f);
    if (!description->SetParameter(splineYID, bc, splineGroupID))
      return false;
  }

  // Add x ramp control
  DescID splineZID = DescLevel(ID_SG_SPLINE_Z, CUSTOMDATATYPE_SPLINE, 0);
  if (!singleID || splineZID.IsPartOf(*singleID, nullptr))
  {
    BaseContainer bc = GetCustomDataTypeDefault(CUSTOMDATATYPE_SPLINE);
    bc.SetString(DESC_NAME, "Z Ramp"_s);
    bc.SetString(DESC_SHORT_NAME, "Z Ramp"_s);
    bc.SetBool(DESC_GUIOPEN, false);
    bc.SetFloat(SPLINECONTROL_X_MIN, 0.0f);
    bc.SetFloat(SPLINECONTROL_X_MAX, 1.0f);
    bc.SetFloat(SPLINECONTROL_Y_MIN, 0.0f);
    bc.SetFloat(SPLINECONTROL_Y_MAX, 1.0f);
    if (!description->SetParameter(splineZID, bc, splineGroupID))
      return false;
  }
  
  flags |= DESCFLAGS_DESC::LOADED;
  return true;
}

Bool SubdivideGrid::Message(GeListNode *node, Int32 type, void *data)
{
  if (type == MSG_DESCRIPTION_CHECKDRAGANDDROP)
  {
    DescriptionCheckDragAndDrop *dcu = (DescriptionCheckDragAndDrop*)data;
    switch (dcu->_descId[0].id)
    {
      case ID_SG_LIST:
        dcu->_result = dcu->_element->IsInstanceOf(Obase);
        return true;
    }
  }
  return true;
};

Bool SubdivideGrid::GetBBox(BaseObject *obj, BaseArray<Vector> &bbox)
{
  iferr_scope_handler { return false; };
  Vector rad = obj->GetRad();
  if (CompareFloatTolerant(rad.GetLength(), 0.0_f) && obj->GetDown()) // if this radius is 0 get children
  {
    BaseArray<BaseObject *> children;
    BaseObject *child = obj->GetDown();
    while (child)
    {
      children.Append(child) iferr_return;
      child = child->GetNext();
    }
    GetCollectiveBBox(children, bbox);
    return true;
  }
  Vector center = (obj->GetUpMg() * obj->GetRelPos()) + obj->GetMp();
  bbox.Append(roundOffVector(center - rad)) iferr_return;
  bbox.Append(roundOffVector(center + rad)) iferr_return;
  return true;
}

Bool SubdivideGrid::GetCollectiveBBox(BaseArray<BaseObject *> &objects, BaseArray<Vector> &bbox)
{
  iferr_scope_handler { return false; };
  GetBBox(objects[0], bbox);
  BaseArray<Vector> tmp;
  for (Int32 x = 1; x < objects.GetCount(); x++)
  {
    GetBBox(objects[x], tmp);
    bbox[0] = MinVector(bbox[0], tmp[0]);
    bbox[1] = MaxVector(bbox[1], tmp[1]);
    tmp.Reset();
  }
  return true;
}

Vector SubdivideGrid::GetRadFromBBox(BaseArray<Vector> &bbox)
{
  Vector rad;
  rad.x = Abs(bbox[1].x - bbox[0].x) / 2;
  rad.y = Abs(bbox[1].y - bbox[0].y) / 2;
  rad.z = Abs(bbox[1].z - bbox[0].z) / 2;
  return rad;
}

Bool SubdivideGrid::GetCornersFromBBox(BaseArray<Vector> &bbox, WritableArrayInterface<Vector> &corners)
{
  iferr_scope_handler { return false; };
  corners.Append(bbox[0]) iferr_return;
  corners.Append(bbox[1]) iferr_return;
  corners.Append(Vector(bbox[0].x, bbox[0].y, bbox[1].z)) iferr_return;
  corners.Append(Vector(bbox[0].x, bbox[1].y, bbox[0].z)) iferr_return;
  corners.Append(Vector(bbox[1].x, bbox[0].y, bbox[0].z)) iferr_return;
  corners.Append(Vector(bbox[1].x, bbox[1].y, bbox[0].z)) iferr_return;
  corners.Append(Vector(bbox[1].x, bbox[0].y, bbox[1].z)) iferr_return;
  corners.Append(Vector(bbox[0].x, bbox[1].y, bbox[1].z)) iferr_return;
  return true;
}

VectorBool SubdivideGrid::MakesFarSides(BaseArray<Vector> &bbox, Vector *farCorner)
{
  VectorBool makesFarSides;
  BaseArray<Vector> corners;
  GetCornersFromBBox(bbox, corners);
  for (Int32 x = 0; x < corners.GetCount(); x++)
  {
    if (CompareFloatTolerant(corners[x].x, farCorner->x))
      makesFarSides.x = true;
    if (CompareFloatTolerant(corners[x].y, farCorner->y))
      makesFarSides.y = true;
    if (CompareFloatTolerant(corners[x].z, farCorner->z))
      makesFarSides.z = true;
  }
  return makesFarSides;
}

EXECUTIONRESULT SubdivideGrid::Execute(BaseTag* tag, BaseDocument* doc, BaseObject* op, BaseThread* bt, Int32 priority, EXECUTIONFLAGS flags)
{
  iferr_scope_handler
  {
    // print error to console
    err.DiagOutput();
    // debug stop
    err.DbgStop();
    return EXECUTIONRESULT::OUTOFMEMORY;
  };
  
  // gather inputs
  GeData completeData(DTYPE_REAL);
  tag->GetParameter(DescID(ID_SG_COMPLETE), completeData, DESCFLAGS_GET::NONE);
  Float complete = completeData.GetFloat();
  
  GeData listData(CUSTOMDATATYPE_INEXCLUDE_LIST);
  tag->GetParameter(DescID(ID_SG_LIST), listData, DESCFLAGS_GET::NONE);
  InExcludeData *splineList = (InExcludeData *)listData.GetCustomDataType(CUSTOMDATATYPE_INEXCLUDE_LIST);
  
  GeData sxData(CUSTOMDATATYPE_SPLINE);
  tag->GetParameter(DescID(ID_SG_SPLINE_X), sxData, DESCFLAGS_GET::NONE);
  SplineData *splineX = (SplineData *)sxData.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
  
  GeData syData(CUSTOMDATATYPE_SPLINE);
  tag->GetParameter(DescID(ID_SG_SPLINE_Y), syData, DESCFLAGS_GET::NONE);
  SplineData *splineY = (SplineData *)syData.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
  
  GeData szData(CUSTOMDATATYPE_SPLINE);
  tag->GetParameter(DescID(ID_SG_SPLINE_Z), szData, DESCFLAGS_GET::NONE);
  SplineData *splineZ = (SplineData *)szData.GetCustomDataType(CUSTOMDATATYPE_SPLINE);
  
  // collect splines
  BaseArray<BaseObject *> splines;
  for (Int32 x = 0; x < splineList->GetObjectCount(); x += 1)
  {
    splines.Append((BaseObject *)splineList->ObjectFromIndex(doc, x)) iferr_return;
  }
  // if nothing in list, get children
  if (splines.GetCount() == 0)
  {
    BaseObject *child = op->GetDown();
    while (child)
    {
      splines.Append(child) iferr_return;
      child = child->GetNext();
    }
  }

  if (splines.GetCount() == 0)
    return EXECUTIONRESULT::OK;

  //  gather parent info so not to recalculate later
  Matrix parentMg = op->GetMg();
  Vector parentAnchor = parentMg.off;

  BaseArray<Vector> parentBbox;
  SortedBBoxCorners parentCorners;
  parentCorners.anchor = parentAnchor;
  GetCollectiveBBox(splines, parentBbox);
  GetCornersFromBBox(parentBbox, parentCorners);
  Vector *parentFarCorner = parentCorners.GetLast();
  Vector parentRad = GetRadFromBBox(parentBbox);
  Vector parentObjSpaceAnchor = parentAnchor - parentBbox[0]; // Anchor in bbox space

  // calculate movements
  for (Int32 x = 0; x < splines.GetCount(); x++)
  {
    BaseObject *spline = splines[x];

    if (!spline->GetDeformMode()) // spline is disabled
      continue;

    BaseArray<Vector> splineBBox;
    GetBBox(spline, splineBBox);
    Vector splineRad = GetRadFromBBox(splineBBox);
    VectorBool makesFarSides = MakesFarSides(splineBBox, parentFarCorner);

    // scale
    Vector maxScaleOff(0.0_f);
    if (makesFarSides.x && !CompareFloatTolerant(splineRad.x, 0.0_f))
      maxScaleOff.x = parentRad.x / splineRad.x;
    if (makesFarSides.y && !CompareFloatTolerant(splineRad.y, 0.0_f))
      maxScaleOff.y = parentRad.y / splineRad.y;
    if (makesFarSides.z && !CompareFloatTolerant(splineRad.z, 0.0_f))
      maxScaleOff.z = parentRad.z / splineRad.z;

    Vector scaleOff(1.0_f);
    scaleOff.x = MapRange(complete, 0.0_f, 1.0_f, maxScaleOff.x, 1.0_f, splineX);
    scaleOff.y = MapRange(complete, 0.0_f, 1.0_f, maxScaleOff.y, 1.0_f, splineY);
    scaleOff.z = MapRange(complete, 0.0_f, 1.0_f, maxScaleOff.z, 1.0_f, splineZ);

    // position
    Vector splineRelPos = spline->GetRelPos();
    Vector maxPosOff = -splineRelPos;
    Vector splineObjSpaceAnchor = (-spline->GetMp()) + splineRad;
    if (makesFarSides.x && !CompareFloatTolerant(splineRad.x, 0.0_f))
    {
      Float origPosX = splineRelPos.x;
      Float newSplineObjSpaceAnchorX = (splineObjSpaceAnchor.x / splineRad.x) * parentRad.x;
      Float newPosX = newSplineObjSpaceAnchorX - parentObjSpaceAnchor.x;
      maxPosOff.x = newPosX - origPosX;
    }
    if (makesFarSides.y && !CompareFloatTolerant(splineRad.y, 0.0_f))
    {
      Float origPosY = splineRelPos.y;
      Float newSplineObjSpaceAnchorY = (splineObjSpaceAnchor.y / splineRad.y) * parentRad.y;
      Float newPosY = newSplineObjSpaceAnchorY - parentObjSpaceAnchor.y;
      maxPosOff.y = newPosY - origPosY;
    }
    if (makesFarSides.z && !CompareFloatTolerant(splineRad.z, 0.0_f))
    {
      Float origPosZ = splineRelPos.z;
      Float newSplineObjSpaceAnchorZ = (splineObjSpaceAnchor.z / splineRad.z) * parentRad.z;
      Float newPosZ = newSplineObjSpaceAnchorZ - parentObjSpaceAnchor.z;
      maxPosOff.z = newPosZ - origPosZ;
    }

    Vector posOff(0);
    posOff.x = MapRange(complete, 0.0_f, 1.0_f, maxPosOff.x, 0.0_f, splineX);
    posOff.y = MapRange(complete, 0.0_f, 1.0_f, maxPosOff.y, 0.0_f, splineY);
    posOff.z = MapRange(complete, 0.0_f, 1.0_f, maxPosOff.z, 0.0_f, splineZ);

    // apply
    spline->SetFrozenPos(posOff);
    spline->SetFrozenScale(scaleOff);
    spline->Message(MSG_UPDATE);
  }

  return EXECUTIONRESULT::OK;
}

//
// MARK: Register
//

Bool RegisterSubdivideGrid()
{
  return RegisterTagPlugin(
                           ID_SUBDIVIDEGRID,
                           "Subdivide Grid"_s,
                           TAG_EXPRESSION | TAG_VISIBLE,
                           SubdivideGrid::Alloc,
                           "Tsubdividegrid"_s,
                           AutoBitmap("subdivide_grid.tiff"_s),
                           0
                           );
}
