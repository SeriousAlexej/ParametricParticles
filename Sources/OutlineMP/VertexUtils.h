#ifndef OUTLINE_VERTEX_UTILS_H
#define OUTLINE_VERTEX_UTILS_H

#define CU_MULT 100.0f

struct GrahamNode
{
  GrahamNode* prev;
  GrahamNode* next;
  PIX2D pos;
  FLOAT3D posAbsolute;
};

struct EntityAndPlacement
{
  CEntity* entity;
  FLOAT3D pos;
  FLOATmatrix3D rot;

  void operator=(CEntity* en);
};

inline void Clear(GrahamNode) {}

extern CDynamicStackArray<GrahamNode> g_points2D;

// returns median of clipped Z components
FLOAT AddTrianglesToCU(const EntityAndPlacement& entity, const CPerspectiveProjection3D& proj);
FLOAT AddEdgesToGraham(const EntityAndPlacement& entity, const CPerspectiveProjection3D& proj);
FLOAT3D ProjectCoordinateReverse(const FLOAT3D& v3dViewPoint, const CPerspectiveProjection3D& proj);

#endif // OUTLINE_VERTEX_UTILS_H
