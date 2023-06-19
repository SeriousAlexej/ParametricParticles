4244
%{
#include "StdH.h"
%}

class SpawnShapeSphere : CEntity {
name "SpawnShapeSphere";
thumbnail "Thumbnails\\SpawnShapeSphere.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Spawn shape Sphere",
  2 FLOAT m_diameter "Diameter" 'D' = 1.0f,

components:
  1 model MODEL_SPHERE "Models\\Editor\\SpawnShapeSphere.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\BoundingBox.tex"

functions:
  FLOAT3D GeneratePosition()
  {
    FLOAT theta = FRnd() * 2.0f * PI;
    FLOAT phi = acos(2.0f * FRnd() - 1.0f);
    FLOAT r = pow(FRnd(), 1.0f/3.0f) * m_diameter * 0.5f;
    FLOAT sinTheta = sin(theta);
    FLOAT cosTheta = cos(theta);
    FLOAT sinPhi = sin(phi);
    FLOAT cosPhi = cos(phi);
    const FLOAT3D pos(r * sinPhi * cosTheta, r * sinPhi * sinTheta, r * cosPhi);
    return pos * GetRotationMatrix() + GetPlacement().pl_PositionVector;
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_diameter *= fStretch;
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_SPHERE);
    SetModelMainTexture(TEXTURE_BBOX);
    if (m_diameter < 0.0f) {
      m_diameter = 0.0f;
    }
    GetModelObject()->StretchModel(FLOAT3D(m_diameter, m_diameter, m_diameter));
    ModelChangeNotify();
  }
};
