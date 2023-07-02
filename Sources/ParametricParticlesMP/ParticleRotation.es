4246
%{
#include "StdH.h"
#include "Particles.h"
%}

class ParticleRotation : CRationalEntity_EnableWeakPointer {
name "ParticleRotation";
thumbnail "Thumbnails\\ParticleRotation.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Particle Rotation",
  2 CEntityPointer m_penNext "Next rotation force (chained)",
  3 FLOAT m_rotationSpeedMin "Rotation speed min" = -10.0f,
  4 FLOAT m_rotationSpeedMax "Rotation speed max" = 10.0f,
  5 FLOAT m_probability "Probability of this rotation" = 1.0f,
  6 BOOL m_speedStretchEdit "Rotation speed stretch..." = FALSE,
  7 CTString m_speedStretch = "0 1",
  8 BOOL m_loop "Loop speed stretch graph" = FALSE,

{
  CStaticArray<FLOAT> speedStretch;
  WeakPointer p_parent;
}

components:
  1 model MODEL_PARTICLE_ROTATION "Models\\Editor\\ParticleRotation.mdl",

functions:
  FLOAT GetSpeedStretchSince(FLOAT birthTime) const
  {
    return GetDiscreteGraphValueSince(speedStretch, birthTime, m_loop);
  }

  void RecacheGraphs()
  {
    RecacheGraphDiscrete(speedStretch, m_speedStretch);
  }

  void Read_t(CTStream* strm)
  {
    CEntity::Read_t(strm);
    p_parent.Read(this, strm);
    RecacheGraphs();
  }

  void Write_t(CTStream* strm)
  {
    CEntity::Write_t(strm);
    p_parent.Write(strm);
  }

  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity* penTarget)
  {
    if (slPropertyOffset == offsetof(ParticleRotation, m_penNext))
    {
      if (!penTarget) {
        return TRUE;
      }
      if (ENTITY_ID(penTarget) == ENTITY_ID(this)) {
        return EnsureNoLoops(this, (const ParticleRotation*)penTarget);
      }
      return FALSE;
    }
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_PARTICLE_ROTATION);

    m_rotationSpeedMax = ClampDn(m_rotationSpeedMax, m_rotationSpeedMin);
    m_probability = Clamp(m_probability, 0.001f, 1.0f);
    if (!IsTargetValid(offsetof(ParticleRotation, m_penNext), m_penNext.ep_pen)) {
      m_penNext = NULL;
    }
    
    EditGraphVariable(this, m_speedStretchEdit, m_speedStretch);
    RecacheGraphs();
    
    ReinitParent(this);
    if (InWED() && m_penNext) {
      ((ParticleRotation*)m_penNext.ep_pen)->p_parent = this;
    }
  }
};
