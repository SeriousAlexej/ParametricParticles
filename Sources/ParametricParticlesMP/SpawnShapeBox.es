4243
%{
#include "StdH.h"
#include "Particles.h"
%}

uses "SpawnShapeBase";

class SpawnShapeBox : SpawnShapeBase {
name "SpawnShapeBox";
thumbnail "Thumbnails\\SpawnShapeBox.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Spawn shape Box",
  2 FLOAT m_sizeX "Size X" 'X' = 1.0f,
  3 FLOAT m_sizeY "Size Y" 'Y' = 1.0f,
  4 FLOAT m_sizeZ "Size Z" 'Z' = 1.0f,
  5 FLOAT m_sizeXinner "Inner Size X" = 0.0f,
  6 FLOAT m_sizeYinner "Inner Size Y" = 0.0f,
  7 FLOAT m_sizeZinner "Inner Size Z" = 0.0f,

components:
  1 model MODEL_BOX "Models\\Editor\\ParametricParticles.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\BoundingBox.tex",
  3 model MODEL_BASE "Models\\Editor\\SpawnShapeBase.mdl",

functions:
  FLOAT3D GeneratePosition()
  {
    FLOAT3D pos;
    if (m_sizeXinner > 0.0f && m_sizeYinner > 0.0f && m_sizeZinner > 0.0f) {
      const FLOAT dx = m_sizeX - m_sizeXinner;
      const FLOAT dy = m_sizeY - m_sizeYinner;
      const FLOAT dz = m_sizeZ - m_sizeZinner;
      const FLOAT xPlatesVolume2 = dx*m_sizeYinner*m_sizeZinner;
      const FLOAT yPlatesVolume2 = dy*m_sizeXinner*m_sizeZinner;
      const FLOAT zPlatesVolume2 = dz*m_sizeXinner*m_sizeYinner;
      const FLOAT xySausagesVolume4 = dx*dy*m_sizeZinner;
      const FLOAT yzSausagesVolume4 = dy*dz*m_sizeXinner;
      const FLOAT xzSausagesVolume4 = dx*dz*m_sizeYinner;
      const FLOAT cornersVolume8 = dx*dy*dz;
      const FLOAT effectiveVolume = m_sizeX*m_sizeY*m_sizeZ - m_sizeXinner*m_sizeYinner*m_sizeZinner;

      const FLOAT plates = xPlatesVolume2 + yPlatesVolume2 + zPlatesVolume2;
      const FLOAT sausages = xySausagesVolume4 + yzSausagesVolume4 + xzSausagesVolume4;
      FLOAT r = FRnd();

      if (r < plates / effectiveVolume && plates > 0.001f)
      {
        // plates
        const INDEX s = IRnd()%2;
        r *= effectiveVolume / plates;
        if (r < xPlatesVolume2 / plates) {
          pos = FLOAT3D(m_sizeXinner*0.5f + FRnd()*0.5f*dx, (FRnd() - 0.5f)*m_sizeYinner, (FRnd() - 0.5f)*m_sizeZinner);
          if (s == 0) {
            pos(1) *= -1;
          }
        } else if (r < (xPlatesVolume2 + yPlatesVolume2) / plates) {
          pos = FLOAT3D((FRnd() - 0.5f)*m_sizeXinner, m_sizeYinner*0.5f + FRnd()*0.5f*dy, (FRnd() - 0.5f)*m_sizeZinner);
          if (s == 0) {
            pos(2) *= -1;
          }
        } else {
          pos = FLOAT3D((FRnd() - 0.5f)*m_sizeXinner, (FRnd() - 0.5f)*m_sizeYinner, m_sizeZinner*0.5f + FRnd()*0.5f*dz);
          if (s == 0) {
            pos(3) *= -1;
          }
        }
      } else if (r < (plates + sausages) / effectiveVolume && sausages > 0.001f)
      {
        // sausages
        const INDEX s = IRnd()%4;
        r *= effectiveVolume / (plates + sausages);
        if (r < xySausagesVolume4 / sausages && xySausagesVolume4 > 0.001f) {
          pos = FLOAT3D(m_sizeXinner*0.5f + FRnd()*0.5f*dx, m_sizeYinner*0.5f + FRnd()*0.5f*dy, (FRnd() - 0.5f)*m_sizeZinner);
          switch (s)
          {
          case 1:
            // X+Y- sausage
            pos(2) *= -1;
            break;
          case 2:
            // X-Y- sausage
            pos(1) *= -1;
            pos(2) *= -1;
            break;
          case 3:
            // X-Y+ sausage
            pos(1) *= -1;
            break;
          }
        } else if (r < (xySausagesVolume4 + yzSausagesVolume4) / sausages && yzSausagesVolume4 > 0.001f) {
          pos = FLOAT3D((FRnd() - 0.5f)*m_sizeXinner, m_sizeYinner*0.5f + FRnd()*0.5f*dy, m_sizeZinner*0.5f + FRnd()*0.5f*dz);
          switch (s)
          {
          case 1:
            // Y+Z- sausage
            pos(3) *= -1;
            break;
          case 2:
            // Y-Z- sausage
            pos(2) *= -1;
            pos(3) *= -1;
            break;
          case 3:
            // Y-Z+ sausage
            pos(2) *= -1;
            break;
          }
        } else {
          pos = FLOAT3D(m_sizeXinner*0.5f + FRnd()*0.5f*dx, (FRnd() - 0.5f)*m_sizeYinner, m_sizeZinner*0.5f + FRnd()*0.5f*dz);
          switch(s)
          {
          case 1:
            // X+Z- sausage
            pos(3) *= -1;
            break;
          case 2:
            // X-Z- sausage
            pos(1) *= -1;
            pos(3) *= -1;
            break;
          case 3:
            // X-Z+ sausage
            pos(1) *= -1;
            break;
          }
        }
      } else {
        // corners
        pos = FLOAT3D(m_sizeXinner*0.5f + FRnd()*0.5f*dx, m_sizeYinner*0.5f + FRnd()*0.5f*dy, m_sizeZinner*0.5f + FRnd()*0.5f*dz);
        switch (IRnd()%8)
        {
        case 1:
          // X+Y+Z- corner
          pos(3) *= -1;
          break;
        case 2:
          // X+Y-Z+ corner
          pos(2) *= -1;
          break;
        case 3:
          // X+Y-Z- corner
          pos(2) *= -1;
          pos(3) *= -1;
          break;
        case 4:
          // X-Y+Z+ corner
          pos(1) *= -1;
          break;
        case 5:
          // X-Y+Z- corner
          pos(1) *= -1;
          pos(3) *= -1;
          break;
        case 6:
          // X-Y-Z+ corner
          pos(1) *= -1;
          pos(2) *= -1;
          break;
        case 7:
          // X-Y-Z- corner
          pos *= -1;
          break;
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

  void SetPlacement_internal(const CPlacement3D& plNew, const FLOATmatrix3D& mRotation, BOOL bNear)
  {
    CEntity::SetPlacement_internal(plNew, mRotation, bNear);
    ReinitParent(this);
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
    
    ReinitParent(this);
  }
};
