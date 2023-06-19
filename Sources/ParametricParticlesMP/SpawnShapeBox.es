4243
%{
#include "StdH.h"
%}

class SpawnShapeBox : CEntity {
name "SpawnShapeBox";
thumbnail "Thumbnails\\SpawnShapeBox.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Spawn shape Box",
  2 FLOAT m_sizeX "Size X" 'X' = 1.0f,
  3 FLOAT m_sizeY "Size Y" 'Y' = 1.0f,
  4 FLOAT m_sizeZ "Size Z" 'Z' = 1.0f,

components:
  1 model MODEL_BOX "Models\\Editor\\ParametricParticles.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\BoundingBox.tex"

functions:
  FLOAT3D GeneratePosition()
  {
    const FLOAT3D pos((FRnd() - 0.5f) * m_sizeX, (FRnd() - 0.5f) * m_sizeY, (FRnd() - 0.5f) * m_sizeZ);
    return pos * GetRotationMatrix() + GetPlacement().pl_PositionVector;
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_sizeX *= fStretch;
    m_sizeY *= fStretch;
    m_sizeZ *= fStretch;

    if (bMirrorX) {
      m_sizeX = -m_sizeX;
    }
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_BOX);
    SetModelMainTexture(TEXTURE_BBOX);
    if (m_sizeX < 0.0f) {
      m_sizeX = 0.0f;
    }
    if (m_sizeY < 0.0f) {
      m_sizeY = 0.0f;
    }
    if (m_sizeZ < 0.0f) {
      m_sizeZ = 0.0f;
    }
    GetModelObject()->StretchModel(FLOAT3D(m_sizeX, m_sizeY, m_sizeZ));
    ModelChangeNotify();
  }
};
