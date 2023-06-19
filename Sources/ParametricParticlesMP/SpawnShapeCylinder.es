4245
%{
#include "StdH.h"
%}

class SpawnShapeCylinder : CEntity {
name "SpawnShapeCylinder";
thumbnail "Thumbnails\\SpawnShapeCylinder.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Spawn shape Cylinder",
  2 FLOAT m_diameter "Diameter" 'D' = 1.0f,
  3 FLOAT m_height "Height" 'H' = 1.0f,

components:
  1 model MODEL_CYLINDER "Models\\Editor\\SpawnShapeCylinder.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\BoundingBox.tex"

functions:
  FLOAT3D GeneratePosition()
  {
    FLOAT theta = FRnd() * 2.0f * PI;
    FLOAT r = sqrt(FRnd()) * m_diameter * 0.5f;
    const FLOAT3D pos(r * cos(theta), (FRnd() - 0.5f) * m_height, r * sin(theta));
    return pos * GetRotationMatrix() + GetPlacement().pl_PositionVector;
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_diameter *= fStretch;
    m_height *= fStretch;
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_CYLINDER);
    SetModelMainTexture(TEXTURE_BBOX);
    if (m_diameter < 0.0f) {
      m_diameter = 0.0f;
    }
    if (m_height < 0.0f) {
      m_height = 0.0f;
    }
    GetModelObject()->StretchModel(FLOAT3D(m_diameter, m_height, m_diameter));
    ModelChangeNotify();
  }
};
