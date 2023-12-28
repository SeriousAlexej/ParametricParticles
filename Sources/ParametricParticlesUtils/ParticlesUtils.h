#ifndef PARAMETRIC_PARTICLES_UTILS_42_H
#define PARAMETRIC_PARTICLES_UTILS_42_H
#include "API.h"

class COneAnim
{
public:
  NAME oa_Name;
  TIME oa_SecsPerFrame;
  INDEX oa_NumberOfFrames;
  INDEX* oa_FrameIndices;
};

#define LIMIT_GRAPH_X 1
#define LIMIT_GRAPH_Y 2

PARAMETRIC_PARTICLES_API FLOAT GetDiscreteGraphValueSince(const CStaticArray<FLOAT>& graph, FLOAT birthTime, BOOL loop);
PARAMETRIC_PARTICLES_API FLOAT GetGraphValueAt(const CStaticArray<FLOAT2D>& graph, FLOAT f);
PARAMETRIC_PARTICLES_API void RecacheGraph(CStaticArray<FLOAT2D>& cache, const CTString& graph);
PARAMETRIC_PARTICLES_API void RecacheGraphDiscrete(CStaticArray<FLOAT>& cache, const CTString& graph);
PARAMETRIC_PARTICLES_API INDEX LowerBound(const CStaticArray<FLOAT2D>& arr, const FLOAT value);
PARAMETRIC_PARTICLES_API void EditGraphVariable(const CEntity* self, BOOL& property, CTString& graph, ULONG flags = 0UL);

#if _MSC_VER == 1200
#define USE_CUSTOM_PARTICLE_PROJECTION
PARAMETRIC_PARTICLES_API CProjection3D* Particle_GetProjection();
#endif

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

#endif // PARAMETRIC_PARTICLES_UTILS_42_H
