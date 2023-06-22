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
  5 FLOAT m_sizeXinner "Size inner X" = 0.0f,
  6 FLOAT m_sizeYinner "Size inner Y" = 0.0f,
  7 FLOAT m_sizeZinner "Size inner Z" = 0.0f,

components:
  1 model MODEL_BOX "Models\\Editor\\ParametricParticles.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\BoundingBox.tex",
  3 model MODEL_BASE "Models\\Editor\\SpawnShapeBase.mdl",

functions:
  FLOAT3D GeneratePosition()
  {
    FLOAT3D pos;
    if (m_sizeXinner > 0.0f && m_sizeYinner > 0.0f && m_sizeZinner > 0.0f) {
      const FLOAT a = FRnd();
      const BOOL s = FRnd() > 0.5f ? TRUE : FALSE;
      if (a <= 0.333333f) {
        pos(1) = m_sizeXinner + (m_sizeX - m_sizeXinner)*FRnd();
        if (s) {
          pos(1) *= -0.5f;
        } else {
          pos(1) *= 0.5f;
        }
        pos(2) = (FRnd() - 0.5f) * m_sizeY;
        pos(3) = (FRnd() - 0.5f) * m_sizeZ;
      } else if (a <= 0.666666f) {
        pos(1) = (FRnd() - 0.5f) * m_sizeX;
        pos(2) = m_sizeYinner + (m_sizeY - m_sizeYinner)*FRnd();
        if (s) {
          pos(2) *= -0.5f;
        } else {
          pos(2) *= 0.5f;
        }
        pos(3) = (FRnd() - 0.5f) * m_sizeZ;
      } else {
        pos(1) = (FRnd() - 0.5f) * m_sizeX;
        pos(2) = (FRnd() - 0.5f) * m_sizeY;
        pos(3) = m_sizeZinner + (m_sizeZ - m_sizeZinner)*FRnd();
        if (s) {
          pos(3) *= -0.5f;
        } else {
          pos(3) *= 0.5f;
        }
      }
    } else {
      pos = FLOAT3D((FRnd() - 0.5f) * m_sizeX, (FRnd() - 0.5f) * m_sizeY, (FRnd() - 0.5f) * m_sizeZ);
    }
    return pos * GetRotationMatrix() + GetPlacement().pl_PositionVector;
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_sizeX *= fStretch;
    m_sizeY *= fStretch;
    m_sizeZ *= fStretch;
    m_sizeXinner *= fStretch;
    m_sizeYinner *= fStretch;
    m_sizeZinner *= fStretch;
  }

procedures:
  Main()
  {
    m_sizeX = ClampDn(m_sizeX, 0.0f);
    m_sizeY = ClampDn(m_sizeY, 0.0f);
    m_sizeZ = ClampDn(m_sizeZ, 0.0f);
    m_sizeXinner = Clamp(m_sizeXinner, 0.0f, m_sizeX);
    m_sizeYinner = Clamp(m_sizeYinner, 0.0f, m_sizeY);
    m_sizeZinner = Clamp(m_sizeZinner, 0.0f, m_sizeZ);

    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_BASE);
    CAttachmentModelObject* outerShape = GetModelObject()->AddAttachmentModel(0);
    CAttachmentModelObject* innerShape = GetModelObject()->AddAttachmentModel(1);
    if (outerShape) {
      CModelObject& outerModel = outerShape->amo_moModelObject;
      outerModel.SetData(GetModelDataForComponent(MODEL_BOX));
      outerModel.mo_toTexture.SetData(GetTextureDataForComponent(TEXTURE_BBOX));
      outerModel.StretchModel(FLOAT3D(m_sizeX, m_sizeY, m_sizeZ));
    }
    if (innerShape) {
      CModelObject& innerModel = innerShape->amo_moModelObject;
      innerModel.SetData(GetModelDataForComponent(MODEL_BOX));
      innerModel.mo_toTexture.SetData(GetTextureDataForComponent(TEXTURE_BBOX));
      innerModel.mo_colBlendColor = CT_OPAQUE;
      innerModel.StretchModel(FLOAT3D(m_sizeXinner, m_sizeYinner, m_sizeZinner));
    }
    GetModelObject()->StretchSingleModel(FLOAT3D(m_sizeX, m_sizeY, m_sizeZ));

    ModelChangeNotify();
  }
};
