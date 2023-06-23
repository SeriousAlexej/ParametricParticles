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
  3 FLOAT m_diameterInner "Inner Diameter" = 0.0f,

components:
  1 model MODEL_SPHERE "Models\\Editor\\SpawnShapeSphere.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\BoundingBox.tex",
  3 model MODEL_BASE "Models\\Editor\\SpawnShapeBase.mdl",

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
    FLOAT3D pos(r * sinPhi * cosTheta, r * sinPhi * sinTheta, r * cosPhi);
    if (m_diameterInner > 0.0f) {
      const FLOAT len = pos.Length();
      if (len > 0.001f) {
        const FLOAT f = len / (m_diameter * 0.5f);
        pos *= ((m_diameterInner*0.5f) + (m_diameter - m_diameterInner)*0.5f*f) / len;
      }
    }
    return pos * GetRotationMatrix() + GetPlacement().pl_PositionVector;
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_diameter *= fStretch;
    m_diameterInner *= fStretch;
  }

procedures:
  Main()
  {
    m_diameter = ClampDn(m_diameter, 0.0f);
    m_diameterInner = Clamp(m_diameterInner, 0.0f, m_diameter);

    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_BASE);
    CAttachmentModelObject* outerShape = GetModelObject()->AddAttachmentModel(0);
    CAttachmentModelObject* innerShape = GetModelObject()->AddAttachmentModel(1);
    if (outerShape) {
      CModelObject& outerModel = outerShape->amo_moModelObject;
      outerModel.SetData(GetModelDataForComponent(MODEL_SPHERE));
      outerModel.mo_toTexture.SetData(GetTextureDataForComponent(TEXTURE_BBOX));
      outerModel.StretchModel(FLOAT3D(m_diameter, m_diameter, m_diameter));
    }
    if (innerShape) {
      CModelObject& innerModel = innerShape->amo_moModelObject;
      innerModel.SetData(GetModelDataForComponent(MODEL_SPHERE));
      innerModel.mo_toTexture.SetData(GetTextureDataForComponent(TEXTURE_BBOX));
      innerModel.mo_colBlendColor = CT_OPAQUE;
      innerModel.StretchModel(FLOAT3D(m_diameterInner, m_diameterInner, m_diameterInner));
    }
    GetModelObject()->StretchSingleModel(FLOAT3D(m_diameter, m_diameter, m_diameter));

    ModelChangeNotify();
  }
};
