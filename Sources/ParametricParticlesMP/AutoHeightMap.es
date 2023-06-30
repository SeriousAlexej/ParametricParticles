4248
%{
#include "StdH.h"
#include "Particles.h"
%}

class AutoHeightMap : CEntity_EnableWeakPointer {
name "AutoHeightMap";
thumbnail "Thumbnails\\AutoHeightMap.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Auto Height Map",
  2 FLOAT m_sizeX "Size X" 'X' = 5.0f,
  3 FLOAT m_sizeY "Size Y" 'Y' = 3.0f,
  4 FLOAT m_sizeZ "Size Z" 'Z' = 5.0f,
  5 FLOAT m_step "Step in meters" = 0.5f,
  6 BOOL m_visible "Visible" = TRUE,
  7 CTString m_heightmap = "",
  8 BOOL m_recalculate "Recalculate" = FALSE,
  9 INDEX m_xSteps = 1,
 10 INDEX m_zSteps = 1,
 11 CEntityPointer m_penNext "Next height map (chained)",

{
  WeakPointer p_parent;
}

components:
  1 model MODEL_BOX "Models\\Editor\\ParametricParticles.mdl",
  2 texture TEXTURE_BBOX "Models\\Editor\\Teleport.tex",

functions:
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity* penTarget)
  {
    if (slPropertyOffset == offsetof(AutoHeightMap, m_penNext))
    {
      if (!penTarget) {
        return TRUE;
      }
      if (penTarget->GetClass()->ec_pdecDLLClass->dec_iID == GetClass()->ec_pdecDLLClass->dec_iID) {
        return EnsureNoLoops(this, (const AutoHeightMap*)penTarget);
      }
      return FALSE;
    }
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  void GenerateHeightmap()
  {
    const BOOL prevEditorModelsRendering = _wrpWorldRenderPrefs.IsEditorModelsOn();
    _wrpWorldRenderPrefs.SetEditorModelsOn(FALSE);

    const INDEX memSize = m_xSteps*m_zSteps + 1;
    FreeMemory(m_heightmap.str_String);
    m_heightmap.str_String = (char*)AllocMemory(memSize);
    m_heightmap.str_String[memSize-1] = 0;
    memset(m_heightmap.str_String, 255, memSize-1);

    const FLOAT3D base(-m_sizeX*0.5f + m_step*0.5f, m_sizeY*0.5f, -m_sizeZ*0.5f + m_step*0.5f);
    const FLOAT3D yAxis = FLOAT3D(0, -1, 0) * GetRotationMatrix();
    for (INDEX x = 0; x < m_xSteps; ++x)
    {
      for (INDEX z = 0; z < m_zSteps; ++z)
      {
        FLOAT3D rayOrigin = base;
        rayOrigin += FLOAT3D(m_step * x, 0, m_step * z);
        rayOrigin = rayOrigin * GetRotationMatrix() + GetPlacement().pl_PositionVector;

        CCastRay ray(this, rayOrigin, yAxis + rayOrigin);
        ray.cr_fHitDistance = m_sizeY;
        ray.cr_bHitTranslucentPortals = FALSE;
        ray.cr_bPhysical = TRUE;
        ray.cr_ttHitModels = CCastRay::TT_FULL;
        ray.cr_fTestR = m_step * 0.5f;
        GetWorld()->CastRay(ray);

        const FLOAT height = m_sizeY - Clamp(ray.cr_fHitDistance, 0.0f, m_sizeY);
        INDEX heightInteger = INDEX(NormFloatToByte(height / m_sizeY)) - 128;
        if (heightInteger == 0) {
          heightInteger -= 1;
        }
        m_heightmap.str_String[x+z*m_xSteps] = heightInteger;
      }
    }
    
    _wrpWorldRenderPrefs.SetEditorModelsOn(prevEditorModelsRendering);
  }

  void RenderParticles()
  {
    if (!m_visible || !_wrpWorldRenderPrefs.IsEditorModelsOn()) {
      return;
    }

    CTextureObject& to = GetModelObject()->mo_toTexture;
    Particle_PrepareTexture(&to, PBT_BLEND);

    const FLOAT3D base(-m_sizeX*0.5f + m_step*0.5f, -m_sizeY*0.5f, -m_sizeZ*0.5f + m_step*0.5f);
    for (INDEX x = 0; x < m_xSteps; ++x)
    {
      for (INDEX z = 0; z < m_zSteps; ++z)
      {
        FLOAT3D pl = base;
        pl += FLOAT3D(m_step * x, 0, m_step * z);
        pl(2) += NormByteToFloat(INDEX(m_heightmap.str_String[x+z*m_xSteps]) + 128) * m_sizeY;
        pl = pl * GetRotationMatrix() + GetPlacement().pl_PositionVector;

        Particle_RenderSquare(pl, m_step*0.5f, 0.0f, C_YELLOW|NormFloatToByte(0.25f));
      }
    }

    Particle_Flush();
  }

  FLOAT BilinearInterpolation(FLOAT a, FLOAT b, FLOAT c, FLOAT d, FLOAT x, FLOAT y) const
  {
    const FLOAT s = Lerp(a, b, x);
    const FLOAT t = Lerp(c, d, x);
    return Lerp(s, t, y);
  }

  FLOAT TrilinearInterpolation(FLOAT a, FLOAT b, FLOAT c, FLOAT d, FLOAT e, FLOAT f, FLOAT g, FLOAT h, FLOAT x, FLOAT y, FLOAT z) const
  {
    const FLOAT s = BilinearInterpolation(a, b, c, d, x, y);
    const FLOAT t = BilinearInterpolation(e, f, g, h, x, y);
    return Lerp(s, t, z);
  }
  
  FLOAT GetAlpha(const FLOAT3D& pos) const
  {
    FLOAT3D p = (pos - GetPlacement().pl_PositionVector) * (!GetRotationMatrix());
    if (Abs(p(1)) > m_sizeX*0.5f ||
        Abs(p(2)) > m_sizeY*0.5f ||
        Abs(p(3)) > m_sizeZ*0.5f) {
      return 1.0f;
    }
    p += FLOAT3D(m_sizeX, m_sizeY-m_step*2.0f, m_sizeZ)*0.5f;

    
    const INDEX xf = Clamp<INDEX>(floor(p(1) / m_step), 0, m_xSteps-1);
    const INDEX zf = Clamp<INDEX>(floor(p(3) / m_step), 0, m_zSteps-1);
    const INDEX xc = Clamp<INDEX>(xf + 1, 0, m_xSteps-1);
    const INDEX zc = Clamp<INDEX>(zf + 1, 0, m_zSteps-1);
    
    const FLOAT yf = floor(p(2) / m_step) * m_step;
    const FLOAT yc = yf + m_step;

    const FLOAT height_ff = NormByteToFloat(INDEX(m_heightmap.str_String[xf+zf*m_xSteps]) + 128) * m_sizeY;
    const FLOAT height_fc = NormByteToFloat(INDEX(m_heightmap.str_String[xf+zc*m_xSteps]) + 128) * m_sizeY;
    const FLOAT height_cf = NormByteToFloat(INDEX(m_heightmap.str_String[xc+zf*m_xSteps]) + 128) * m_sizeY;
    const FLOAT height_cc = NormByteToFloat(INDEX(m_heightmap.str_String[xc+zc*m_xSteps]) + 128) * m_sizeY;

    const FLOAT p000(height_ff >= yf ? 0.0f : 1.0f);
    const FLOAT p001(height_fc >= yf ? 0.0f : 1.0f);
    const FLOAT p010(height_ff >= yc ? 0.0f : 1.0f);
    const FLOAT p011(height_fc >= yc ? 0.0f : 1.0f);
    const FLOAT p100(height_cf >= yf ? 0.0f : 1.0f);
    const FLOAT p101(height_cc >= yf ? 0.0f : 1.0f);
    const FLOAT p110(height_cf >= yc ? 0.0f : 1.0f);
    const FLOAT p111(height_cc >= yc ? 0.0f : 1.0f);

    const FLOAT x = (p(1) - xf*m_step) / m_step;
    const FLOAT y = (p(2) - yf) / m_step;
    const FLOAT z = (p(3) - zf*m_step) / m_step;

    return TrilinearInterpolation(
      p000, p100,
      p010, p110,

      p001, p101,
      p011, p111,
      x, y, z);
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_sizeX *= fStretch;
    m_sizeY *= fStretch;
    m_sizeZ *= fStretch;
    m_step *= fStretch;
    m_xSteps = m_sizeX / m_step;
    m_zSteps = m_sizeZ / m_step;
    GenerateHeightmap();
  }

  void Read_t(CTStream* strm)
  {
    CEntity::Read_t(strm);
    p_parent.Read(this, strm);
  }

  void Write_t(CTStream* strm)
  {
    CEntity::Write_t(strm);
    p_parent.Write(strm);
  }

  void SetPlacement_internal(const CPlacement3D& plNew, const FLOATmatrix3D& mRotation, BOOL bNear)
  {
    CEntity::SetPlacement_internal(plNew, mRotation, bNear);
    ReinitParent(this);
  }

procedures:
  Main()
  {
    m_sizeX = ClampDn(m_sizeX, 1.0f);
    m_sizeY = ClampDn(m_sizeY, 1.0f);
    m_sizeZ = ClampDn(m_sizeZ, 1.0f);
    m_step = ClampUp(m_step, Min(Min(m_sizeX, m_sizeY), m_sizeZ));
    m_recalculate = FALSE;
    m_xSteps = m_sizeX / m_step;
    m_zSteps = m_sizeZ / m_step;
    if (!IsTargetValid(offsetof(AutoHeightMap, m_penNext), m_penNext.ep_pen)) {
      m_penNext = NULL;
    }

    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_BOX);
    SetModelMainTexture(TEXTURE_BBOX);
    GetModelObject()->mo_colBlendColor = C_YELLOW|NormFloatToByte(0.25f);
    GetModelObject()->StretchModel(FLOAT3D(m_sizeX, m_sizeY, m_sizeZ));
    ModelChangeNotify();

    GenerateHeightmap();
    
    ReinitParent(this);
    if (_bWorldEditorApp && m_penNext) {
      ((AutoHeightMap*)m_penNext.ep_pen)->p_parent = this;
    }
  }
};
