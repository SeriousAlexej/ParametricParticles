4245
%{
#include "StdH.h"
#include "Particles.h"
%}

uses "SpawnShapeBase";

class SpawnShapeCylinder : SpawnShapeBase {
name "SpawnShapeCylinder";
thumbnail "Thumbnails\\SpawnShapeCylinder.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Spawn shape Cylinder",
  2 FLOAT m_diameter "Diameter" 'D' = 1.0f,
  3 FLOAT m_height "Height" 'H' = 1.0f,
  4 FLOAT m_diameterInner "Inner Diameter" = 0.0f,
  5 FLOAT m_heightInner "Inner Height" = 0.0f,

components:
  1 model MODEL_CYLINDER "Models\\Editor\\SpawnShapeCylinder.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\BoundingBox.tex",
  3 model MODEL_BASE "Models\\Editor\\SpawnShapeBase.mdl",

functions:
  FLOAT3D GeneratePosition()
  {
    FLOAT theta = FRnd() * 2.0f * PI;
    FLOAT r = sqrt(FRnd());
    FLOAT3D pos;
    if (m_diameterInner > 0.0f && m_heightInner > 0.0f) {
      const FLOAT innerVolume = m_diameterInner*m_diameterInner*0.25*PI*m_heightInner;
      const FLOAT circleArea = m_diameter*m_diameter*0.25*PI;
      const FLOAT fullVolume = circleArea*m_height;
      const FLOAT capsVolume = circleArea*(m_height-m_heightInner);
      const FLOAT sideVolume = fullVolume - capsVolume - innerVolume;
      const FLOAT effectiveVolume = capsVolume + sideVolume;
      if (FRnd() <= sideVolume / effectiveVolume) {
        pos = FLOAT3D(r * cos(theta), 0.0f, r * sin(theta));
        const FLOAT len = pos.Length();
        if (len > 0.001f) {
          pos = (pos / len) * ((m_diameterInner*0.5f) + (m_diameter - m_diameterInner)*0.5f*len);
        }
        pos(2) = (FRnd() - 0.5f) * m_heightInner;
      } else {
        r *= m_diameter * 0.5f;
        pos = FLOAT3D(r * cos(theta), m_heightInner*0.5f + FRnd()*(m_height-m_heightInner)*0.5f, r * sin(theta));
        if (FRnd() < 0.5f) {
          pos(2) *= -1.0f;
        }
      }
    } else {
      r *= m_diameter * 0.5f;
      pos = FLOAT3D(r * cos(theta), (FRnd() - 0.5f) * m_height, r * sin(theta));
    }
    return pos * GetRotationMatrix() + GetPlacement().pl_PositionVector;
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_diameter *= fStretch;
    m_height *= fStretch;
    m_diameterInner *= fStretch;
    m_heightInner *= fStretch;
  }

  void SetPlacement_internal(const CPlacement3D& plNew, const FLOATmatrix3D& mRotation, BOOL bNear)
  {
    CEntity::SetPlacement_internal(plNew, mRotation, bNear);
    ReinitParent(this);
  }

procedures:
  Main()
  {
    m_diameter = ClampDn(m_diameter, 0.0f);
    m_height = ClampDn(m_height, 0.0f);
    m_diameterInner = Clamp(m_diameterInner, 0.0f, m_diameter);
    m_heightInner = Clamp(m_heightInner, 0.0f, m_height);

    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_BASE);
    CAttachmentModelObject* outerShape = GetModelObject()->AddAttachmentModel(0);
    CAttachmentModelObject* innerShape = GetModelObject()->AddAttachmentModel(1);
    if (outerShape) {
      CModelObject& outerModel = outerShape->amo_moModelObject;
      outerModel.SetData(GetModelDataForComponent(MODEL_CYLINDER));
      outerModel.mo_toTexture.SetData(GetTextureDataForComponent(TEXTURE_BBOX));
      outerModel.StretchModel(FLOAT3D(m_diameter, m_height, m_diameter));
    }
    if (innerShape) {
      CModelObject& innerModel = innerShape->amo_moModelObject;
      innerModel.SetData(GetModelDataForComponent(MODEL_CYLINDER));
      innerModel.mo_toTexture.SetData(GetTextureDataForComponent(TEXTURE_BBOX));
      innerModel.mo_colBlendColor = CT_OPAQUE;
      innerModel.StretchModel(FLOAT3D(m_diameterInner, m_heightInner, m_diameterInner));
    }
    GetModelObject()->StretchSingleModel(FLOAT3D(m_diameter, m_height, m_diameter));

    ModelChangeNotify();

    ReinitParent(this);
  }
};
