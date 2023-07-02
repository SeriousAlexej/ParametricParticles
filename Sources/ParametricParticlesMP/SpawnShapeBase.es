4249
%{
#include "StdH.h"
#include "Particles.h"
%}

class SpawnShapeBase : CRationalEntity_EnableWeakPointer
{
name "SpawnShapeBase";
thumbnail "";
properties:
  1 CEntityPointer m_penNext "Next spawner shape (chained)",

{
  WeakPointer p_parent;
}
components:
functions:
  virtual FLOAT GetVolume() const
  {
    return 0.0f;
  }

  virtual FLOAT3D GeneratePosition()
  {
    return GetPlacement().pl_PositionVector;
  }

  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity* penTarget)
  {
    if (slPropertyOffset == offsetof(SpawnShapeBase, m_penNext))
    {
      if (!penTarget) {
        return TRUE;
      }
      if (ANCESTOR_ID(penTarget) == ANCESTOR_ID(this)) {
        return EnsureNoLoops(this, (const SpawnShapeBase*)penTarget);
      }
      return FALSE;
    }
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  void ValidateNextPointer()
  {
    if (!IsTargetValid(offsetof(SpawnShapeBase, m_penNext), m_penNext.ep_pen)) {
      m_penNext = NULL;
    }
    ReinitParent(this);
    if (InWED() && m_penNext) {
      ((SpawnShapeBase*)m_penNext.ep_pen)->p_parent = this;
    }
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

  void SetPlacement_internal(const CPlacement3D& plNew, const FLOATmatrix3D& mRotation, BOOL bNear)
  {
    CEntity::SetPlacement_internal(plNew, mRotation, bNear);
    ReinitParent(this);
  }

procedures:
  Main()
  {
  }
};
