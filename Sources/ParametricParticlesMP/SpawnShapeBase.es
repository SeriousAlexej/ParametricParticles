4249
%{
#include "StdH.h"
%}

class SpawnShapeBase : CRationalEntity_EnableWeakPointer
{
name "SpawnShapeBase";
thumbnail "";
properties:
{
  WeakPointer p_parent;
}
components:
functions:
  virtual FLOAT3D GeneratePosition()
  {
    return GetPlacement().pl_PositionVector;
  }

  void Read_t(CTStream* strm)
  {
    CEntity::Read_t(strm); // must be CEntity for compatibility
    p_parent.Read(this, strm);
  }

  void Write_t(CTStream* strm)
  {
    CEntity::Write_t(strm); // must be CEntity for compatibility
    p_parent.Write(strm);
  }
procedures:
  Main()
  {
  }
};
