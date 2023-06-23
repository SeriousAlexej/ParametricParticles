#ifndef PARAMETRIC_PARTICLES_42_H
#define PARAMETRIC_PARTICLES_42_H

#define LIMIT_GRAPH_X 1
#define LIMIT_GRAPH_Y 2

class ParametricParticles;

FLOAT GetDiscreteGraphValueSince(const CStaticArray<FLOAT>& graph, FLOAT birthTime, BOOL loop);
FLOAT GetGraphValueAt(const CStaticArray<FLOAT2D>& graph, FLOAT f);
void RecacheGraph(CStaticArray<FLOAT2D>& cache, const CTString& graph);
void RecacheGraphDiscrete(CStaticArray<FLOAT>& cache, const CTString& graph);
INDEX LowerBound(const CStaticArray<FLOAT2D>& arr, const FLOAT value);
void EditGraphVariable(const CEntity* self, BOOL& property, CTString& graph, ULONG flags = 0UL);

#if _MSC_VER == 1200
#define USE_CUSTOM_PARTICLE_PROJECTION
CProjection3D* Particle_GetProjection();
#endif

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
  void Write(CTStream* strm);
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
};

template<typename TClass>
BOOL EnsureNoLoops(const TClass* pthis, const TClass* pthat)
{
  CStaticStackArray<const CEntity*> visited;
  visited.Push() = pthis;

  const TClass* toAdd = pthat;
  while (toAdd)
  {
    for (INDEX i = 0; i < visited.Count(); ++i)
      if (visited[i] == toAdd)
        return FALSE;
    visited.Push() = (const CEntity*)toAdd;
    toAdd = (const TClass*)toAdd->m_penNext.ep_pen;
  }
  return TRUE;
}

#endif // PARAMETRIC_PARTICLES_42_H
