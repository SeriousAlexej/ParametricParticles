4247
%{
#include "StdH.h"
#include "Particles.h"
%}

class ParticleVelocity : CEntity {
name "ParticleVelocity";
thumbnail "Thumbnails\\ParticleVelocity.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Particle Velocity",
  2 CEntityPointer m_penNext "Next velocity force (chained)",
  3 FLOAT m_probability "Probability of this velocity" = 1.0f,
  4 BOOL m_speedStretchEdit "Velocity speed stretch..." = FALSE,
  5 CTString m_speedStretch = "0 1",
  6 FLOAT m_randomHMin "Velocity random heading min" = 0.0f,
  7 FLOAT m_randomHMax "Velocity random heading max" = 0.0f,
  8 FLOAT m_randomPMin "Velocity random pitch min" = 0.0f,
  9 FLOAT m_randomPMax "Velocity random pitch max" = 0.0f,
 10 BOOL m_loop "Loop speed stretch graph" = FALSE,

{
  CStaticArray<FLOAT> speedStretch;
}

components:
  1 model MODEL_PARTICLE_VELOCITY "Models\\Editor\\ParticleVelocity.mdl",

functions:
  ANGLE3D GetRandomAngles(FLOAT r1, FLOAT r2) const
  {
    return ANGLE3D(
      m_randomHMin + r1 * (m_randomHMax - m_randomHMin),
      m_randomPMin + r2 * (m_randomPMax - m_randomPMin),
      0);
  }

  FLOAT GetVelocityStretchSince(FLOAT birthTime) const
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
    RecacheGraphs();
  }

  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity* penTarget)
  {
    if (slPropertyOffset == offsetof(ParticleVelocity, m_penNext))
    {
      if (!penTarget) {
        return TRUE;
      }
      if (penTarget->GetClass()->ec_pdecDLLClass->dec_iID == GetClass()->ec_pdecDLLClass->dec_iID) {
        return EnsureNoLoops(this, (const ParticleVelocity*)penTarget);
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
    SetModel(MODEL_PARTICLE_VELOCITY);

    m_randomHMin = Clamp(m_randomHMin, -180.0f, 180.0f);
    m_randomHMax = ClampDn(m_randomHMax, m_randomHMin);
    m_randomPMin = Clamp(m_randomPMin, -180.0f, 180.0f);
    m_randomPMax = ClampDn(m_randomPMax, m_randomPMin);
    m_probability = Clamp(m_probability, 0.001f, 1.0f);
    if (!IsTargetValid(offsetof(ParticleVelocity, m_penNext), m_penNext.ep_pen)) {
      m_penNext = NULL;
    }
    
    EditGraphVariable(this, m_speedStretchEdit, m_speedStretch);
    RecacheGraphs();
  }
};
