#ifndef PARAMETRIC_PARTICLES_42_H
#define PARAMETRIC_PARTICLES_42_H
#include <ParametricParticlesUtils/ParticlesUtils.h>

class ParametricParticles;

extern BOOL g_parentIsPredictor;
extern BOOL g_parentRelativePlacement;
extern FLOAT3D g_projectionZAxis;
extern FLOAT3D g_viewerLerpedPositionForComparison;
extern FLOAT3D g_parentLerpedPositionForComparison;
extern FLOATmatrix3D g_parentLerpedRotationForComparison;

struct Particle
{
  Particle();
  FLOAT RandomFloat(ULONG& rndSeed) const;
  BOOL IsAlive() const;
  BOOL IsVisible(const ParametricParticles* parent) const;
  void Update(const ParametricParticles* parent);
  void Create(ParametricParticles* parent);
  void Render(ParametricParticles* parent) const;
  void Write(CTStream* strm) const;
  void Read(CTStream* strm);
  FLOAT3D LerpedPos() const;
  FLOAT LerpedRot() const;
  static int CompareID(const void* lhs, const void* rhs);
  static int CompareDistance(const void* lhs, const void* rhs);

  Particle* next; // next & prev "free or used particle", no particular order
  Particle* prev;
  INDEX id; // used only to sort particles for Write(CTStream)

  FLOAT3D position;
  FLOAT3D lastPosition;
  FLOAT2D size;
  FLOAT rotation;
  FLOAT lastRotation;
  INDEX tileRow;
  INDEX tileCol;
  TIME birthTime;
  TIME deathTime;
  ULONG savedRndSeed;

  mutable FLOAT additionalRotation;
};

template<typename TClass>
void ReinitParent(TClass* self)
{
  ReinitParent(self->p_parent, self);
}

void ReinitParent(WeakPointer& p_parent, CEntity* self);

#endif // PARAMETRIC_PARTICLES_42_H
