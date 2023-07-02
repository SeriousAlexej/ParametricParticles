4242
%{
#include "StdH.h"
#include "Particles.h"
#include "AutoHeightMap.h"
#include "ParticleVelocity.h"
#include "ParticleRotation.h"
#include "SpawnShapeBox.h"
#include "SpawnShapeCylinder.h"
#include "SpawnShapeSphere.h"
#define ID_PARAMETRIC_PARTICLES "PPAR"

class COneAnim
{
public:
  NAME oa_Name;
  TIME oa_SecsPerFrame;
  INDEX oa_NumberOfFrames;
  INDEX* oa_FrameIndices;
};
%}

enum eParticleBlendType
{
  0 ePBT_BLEND "Blend",
  1 ePBT_ADD "Add",
  2 ePBT_MULTIPLY "Multiply",
  3 ePBT_ADDALPHA "Add alpha",
  4 ePBT_FLEX "Flex",
  5 ePBT_TRANSPARENT "Transparent",
};

enum eFlat
{
  0 FLAT_FULL "Full (face camera)",
  1 FLAT_HALF "Half (heading to camera)",
  2 FLAT_NONE "None (fixed orientation)",
};

enum eTextureType
{
  0 TT_RANDOM_FRAME "Random tile",
  1 TT_ANIMATED "Animated tiles",
};

enum eParticlePlacement
{
  0 PP_ABSOLUTE "Absolute",
  1 PP_RELATIVE "Relative",
};

class ParametricParticles : CRationalEntity_EnableWeakPointer
{
name "ParametricParticles";
thumbnail "Thumbnails\\ParametricParticles.tbn";
features "HasName", "IsTargetable";

properties:
  1 CTString m_strName "Name" 'N' = "Parametric Particles",
  2 FLOAT m_fStretchX "Clipping box size X (for rendering only)" = 1.0f,
  3 FLOAT m_fStretchY "Clipping box size Y" = 1.0f,
  4 FLOAT m_fStretchZ "Clipping box size Z" = 1.0f,
  5 BOOL m_bActive "Active" 'A' = TRUE,
  6 FLOAT m_fSpawnInterval "Spawn interval (seconds)" = 0.1f,
  7 INDEX m_iSpawnMin "Spawn count min" = 0,
  8 INDEX m_iSpawnMax "Spawn count max" = 1,
  9 FLOAT m_fLastSpawnTime = 0.0f,
 10 CTFileName m_fnTexture "Texture" 'T' = CTFILENAME("Models\\SpecularTextures\\Medium.tex"),
 11 INDEX m_freeID = 0,
 12 FLOAT m_fParticleLifetimeMin "Particle lifetime min" = 5.0f,
 13 FLOAT m_fParticleLifetimeMax "Particle lifetime max" = 6.0f,
 14 INDEX m_iTextureNumRows "Texture rows" = 1,
 15 INDEX m_iTextureNumCols "Texture columns" = 1,
 16 CEntityPointer m_penSpawnerShape "Spawner shape",
 17 enum eParticleBlendType m_blendType "Blend type" = ePBT_BLEND,
 18 enum eFlat m_flatType "Flat type" = FLAT_FULL,
 19 BOOL m_editAlpha "Particle alpha..." = FALSE,
 20 CTString m_alphaGraph = "0 1",
 21 BOOL m_editStretchX "Particle stretch X..." = FALSE,
 22 CTString m_stretchXGraph = "0 1",
 23 BOOL m_editStretchY "Particle stretch Y..." = FALSE,
 24 CTString m_stretchYGraph = "0 1",
 25 FLOAT m_sizeX "Particle size X min" = 1.0f,
 26 FLOAT m_sizeY "Particle size Y min" = 1.0f,
 27 COLOR m_color "Particle color base" = C_WHITE,
 28 enum eTextureType m_textureType "Texture type" = TT_RANDOM_FRAME,
 29 FLOAT m_textureSpeed "Texture animation FPS" = 24.0f,
 30 INDEX m_textureTiles "Texture tiles count" = 1,
 31 FLOAT m_maxsizeX "Particle size X max" = 1.0f,
 32 FLOAT m_maxsizeY "Particle size Y max" = 1.0f,
 33 BOOL m_sizeUniform "Particle size uniform" = TRUE,
 34 enum eParticlePlacement m_particlePlacement "Particle placement" = PP_ABSOLUTE,
 35 FLOAT m_rotationMin "Particle rotation min" = 0.0f,
 36 FLOAT m_rotationMax "Particle rotation max" = 0.0f,
 37 CEntityPointer m_penRotation "Particle rotation force (chained)",
 38 CEntityPointer m_penVelocity "Particle velocity force (chained)",
 39 BOOL m_background "Background" = FALSE,
 40 BOOL m_help "Online Help..." = FALSE,
 41 ANIMATION m_textureAnimation "Texture animation" = 0,
 42 INDEX m_maxParticles "Alive particles max count" = 512,
 43 RANGE m_particleFallOff "Particle visibility fall-off" = 30.0f,
 44 RANGE m_particleHotSpot "Particle visibility hot-spot" = 25.0f,
 45 FLOAT m_updateStep = 1.0f,
 46 FLOAT m_presimulationSpan "Presimulate time span" = 3.0f,
 47 BOOL m_presimulated = FALSE,
 48 CEntityPointer m_penHeightMap "Height maps (chained)",
 49 CAnimObject m_colorAnimationObject,
 50 ANIMATION m_colorAnimation "Particle color animation" = 0,
 51 CTFileName m_colorAnimationFile "Particle color animation file" = CTString(""),
 52 BOOL m_followVelocity "Orient towards velocity" = FALSE,

{
  Particle* lastFree;
  Particle* lastUsed;
  CStaticArray<Particle> particles;
  CStaticArray<Particle*> renderParticles;
  CStaticArray<FLOAT2D> alphaCache;
  CStaticArray<FLOAT2D> stretchXCache;
  CStaticArray<FLOAT2D> stretchYCache;
}

components:
  1 model BOX_MODEL "Models\\Editor\\ParametricParticles.mdl"

functions:
  CAnimData* GetAnimData(SLONG slPropertyOffset)
  {
    if (slPropertyOffset == offsetof(ParametricParticles, m_textureAnimation)) {
      return GetModelObject()->mo_toTexture.GetData();
    } else if (slPropertyOffset == offsetof(ParametricParticles, m_colorAnimation)) {
      return m_colorAnimationObject.GetData();
    }
    return CRationalEntity_EnableWeakPointer::GetAnimData(slPropertyOffset);
  }

  void RecacheArrays()
  {
    particles.Clear();
    particles.New(m_maxParticles);
    renderParticles.Clear();
    renderParticles.New(m_maxParticles);
    
    lastFree = NULL;
    lastUsed = NULL;
    for (INDEX i = 0; i < m_maxParticles; ++i) {
      PushFree(&particles[i]);
    }

    RecacheGraph(alphaCache, m_alphaGraph);
    RecacheGraph(stretchXCache, m_stretchXGraph);
    RecacheGraph(stretchYCache, m_stretchYGraph);
  }

  COLOR GetColorSince(FLOAT birthTime) const
  {
    COLOR col = m_color;

    const CAnimData* animData = m_colorAnimationObject.ao_AnimData;
    if (animData && m_colorAnimation < animData->ad_NumberOfAnims)
    {
      const COneAnim& anim = animData->ad_Anims[m_colorAnimation];
      if (anim.oa_NumberOfFrames > 0)
      {
        const TIME tmCurrentRelative = _pTimer->GetLerpedCurrentTick() - birthTime;
        const FLOAT fFrameNow = tmCurrentRelative / anim.oa_SecsPerFrame;
        const INDEX col0 = anim.oa_FrameIndices[ULONG(fFrameNow) % anim.oa_NumberOfFrames];
        const INDEX col1 = anim.oa_FrameIndices[ULONG(fFrameNow+1) % anim.oa_NumberOfFrames];
        const FLOAT f = fFrameNow - floor(fFrameNow);

        UBYTE R0, G0, B0;
        UBYTE R1, G1, B1;
        ColorToRGB(col0, R0, G0, B0);
        ColorToRGB(col1, R1, G1, B1);

        col = MulColors(col, RGBToColor(Lerp(R0, R1, f), Lerp(G0, G1, f), Lerp(B0, B1, f)));
      }
    }
    return col & C_WHITE;
  }

  FLOAT GetAlpha(FLOAT f, const FLOAT3D& p) const
  {
    FLOAT alpha = Clamp(GetGraphValueAt(alphaCache, f), 0.0f, 1.0f);
    const AutoHeightMap* heightMap = (const AutoHeightMap*)m_penHeightMap.ep_pen;
    while (alpha > 0.0001f && heightMap)
    {
      alpha *= heightMap->GetAlpha(p);
      heightMap = (const AutoHeightMap*)heightMap->m_penNext.ep_pen;
    }
    return alpha;
  }

  FLOAT GetXStretch(FLOAT f) const
  {
    return GetGraphValueAt(stretchXCache, f);
  }

  FLOAT GetYStretch(FLOAT f) const
  {
    return GetGraphValueAt(stretchYCache, f);
  }

  void ParametricParticles()
  {
    lastFree = NULL;
    lastUsed = NULL;
  }
  
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity* penTarget)
  {
    if (slPropertyOffset == offsetof(ParametricParticles, m_penHeightMap))
    {
      if (!penTarget) {
        return TRUE;
      }
      return (ENTITY_ID(penTarget) == ID_AutoHeightMap) ? TRUE : FALSE;
    }

    if (slPropertyOffset == offsetof(ParametricParticles, m_penVelocity))
    {
      if (!penTarget) {
        return TRUE;
      }
      return (ENTITY_ID(penTarget) == ID_ParticleVelocity) ? TRUE : FALSE;
    }

    if (slPropertyOffset == offsetof(ParametricParticles, m_penRotation))
    {
      if (!penTarget) {
        return TRUE;
      }
      return (ENTITY_ID(penTarget) == ID_ParticleRotation) ? TRUE : FALSE;
    }
    
    if (slPropertyOffset == offsetof(ParametricParticles, m_penSpawnerShape))
    {
      if (!penTarget) {
        return TRUE;
      }
      switch (ENTITY_ID(penTarget))
      {
      case ID_SpawnShapeBox:
      case ID_SpawnShapeSphere:
      case ID_SpawnShapeCylinder:
        return TRUE;
      }
      return FALSE;
    }

    return CRationalEntity_EnableWeakPointer::IsTargetValid(slPropertyOffset, penTarget);
  }

  void Read_t(CTStream* strm)
  { 
    CRationalEntity_EnableWeakPointer::Read_t(strm);

    // leftover from CMovableModelEntity
    if (strm->PeekID_t() == CChunkID("MENT"))
    {
      strm->ExpectID_t("MENT");
      INDEX dummy;
      INDEX dummy2;
      (*strm) >> dummy;
      (*strm) >> dummy;
      for(INDEX i = 0; i < dummy; ++i) {
        (*strm) >> dummy2;
      }
      (*strm) >> dummy;
    }

    RecacheArrays();
    
    strm->ExpectID_t(ID_PARAMETRIC_PARTICLES);

    INDEX numActive = 0;
    (*strm) >> numActive;

    lastUsed = NULL;
    if (numActive > 0) {
      {for (INDEX i = 0; i < numActive; ++i) {
        particles[i].Read(strm);
      }}
      particles[0].prev = NULL;
      {for (INDEX i = 1; i < numActive; ++i) {
        Particle& lhs = particles[i - 1];
        Particle& rhs = particles[i];
        lhs.next = &rhs;
        rhs.prev = &lhs;
      }}
      particles[numActive - 1].next = NULL;
      lastUsed = &particles[numActive - 1];
    }

    lastFree = NULL;
    {for (INDEX i = numActive; i < m_maxParticles; ++i) {
      particles[i].prev = NULL;
      particles[i].next = NULL;
    }}
    {for (INDEX i = numActive; i < m_maxParticles; ++i) {
      PushFree(&particles[i]);
    }}
  }
  
  void Write_t(CTStream* strm)
  {
    CRationalEntity_EnableWeakPointer::Write_t(strm);
    
    strm->WriteID_t(CChunkID(ID_PARAMETRIC_PARTICLES));

    CStaticArray<Particle*> activeParticles;
    activeParticles.New(m_maxParticles);

    INDEX numActive = 0;
    Particle* particle = lastUsed;
    while (particle) {
      activeParticles[numActive++] = particle;
      particle = particle->prev;
    }
    qsort(activeParticles.sa_Array, numActive, sizeof(Particle*), &Particle::CompareID);
    (*strm) << numActive;
    for (INDEX i = 0; i < numActive; ++i) {
      activeParticles[i]->Write(strm);
    }
  }

  void PushFree(Particle* particle)
  {
    if (particle->prev) {
      particle->prev->next = particle->next;
    }
    if (particle->next) {
      particle->next->prev = particle->prev;
    }
    if (particle == lastUsed) {
      lastUsed = particle->prev;
    }

    if (lastFree) {
      lastFree->next = particle;
    }
    particle->prev = lastFree;
    lastFree = particle;
    particle->next = NULL;
  }

  Particle* PopFree()
  {
    if (!lastFree) {
      return NULL;
    }
    Particle* result = lastFree;
    lastFree = result->prev;
    if (lastFree) {
      lastFree->next = NULL;
    }

    if (lastUsed) {
      lastUsed->next = result;
    }
    result->prev = lastUsed;
    lastUsed = result;
    result->next = NULL;
    return result;
  }

  BOOL HasAliveParticles() const
  {
    return lastUsed ? TRUE : FALSE;
  }

  void UpdateParticles()
  {
    FLOATaabbox3D box;
    Particle* particle = lastUsed;
    while (particle)
    {
      Particle* current = particle;
      particle = current->prev;

      if (current->IsAlive()) {
        current->Update(this);
        if (m_particlePlacement == PP_RELATIVE) {
          box |= current->position * GetRotationMatrix() + GetPlacement().pl_PositionVector;
        } else {
          box |= current->position;
        }
      } else {
        PushFree(current);
      }
    }

    if (m_bActive && _pTimer->CurrentTick() >= m_fLastSpawnTime + m_fSpawnInterval)
    {
      m_fLastSpawnTime = _pTimer->CurrentTick();
      const INDEX numToSpawn = m_iSpawnMin + (IRnd() % (m_iSpawnMax - m_iSpawnMin + 1));
      for (INDEX i = 0; i < numToSpawn; ++i)
      {
        Particle* newParticle = PopFree();
        if (!newParticle) {
          break;
        }
        newParticle->Create(this);
        if (m_particlePlacement == PP_RELATIVE) {
          box |= newParticle->position * GetRotationMatrix() + GetPlacement().pl_PositionVector;
        } else {
          box |= newParticle->position;
        }
      }
    }

    box.Expand(m_particleFallOff + 20.0f);

    if (m_background) {
      return;
    }

    for (INDEX i = 0; i < GetMaxPlayers(); ++i)
    {
      const CEntity* penPlayer = GetPlayerEntity(i);
      if (!penPlayer) {
        continue;
      }
      const FLOAT3D& plPos = penPlayer->GetPlacement().pl_PositionVector;
      if (box.HasContactWith(plPos)) {
        m_updateStep = CTimer::TickQuantum;
        return;
      }
    }
    m_updateStep = 2.0f + FRnd();
  }

  void RenderParticles()
  {
    if (!HasAliveParticles()) {
      return;
    }
    const CProjection3D* proj = Particle_GetProjection();
    if (!proj) {
      return;
    }

    g_parentIsPredictor = IsPredictor();
    g_parentRelativePlacement = m_particlePlacement == PP_RELATIVE ? TRUE : FALSE;
    g_projectionZAxis = Particle_GetProjection()->pr_ViewerRotationMatrix.GetRow(3);
    const CPlacement3D lerpedPlacement = GetLerpedPlacement();
    g_parentLerpedPositionForComparison = lerpedPlacement.pl_PositionVector;
    MakeRotationMatrixFast(g_parentLerpedRotationForComparison, lerpedPlacement.pl_OrientationAngle);
    g_viewerLerpedPositionForComparison = proj->pr_ViewerPlacement.pl_PositionVector;

    INDEX numParticles = 0;
    Particle* particle = lastUsed;
    while (particle)
    {
      if (m_background || particle->IsVisible(this)) {
        renderParticles[numParticles++] = particle;
      }
      particle = particle->prev;
    }
    if (numParticles == 0) {
      return;
    }

    qsort(renderParticles.sa_Array, numParticles, sizeof(Particle*), &Particle::CompareDistance);

    CTextureObject& to = GetModelObject()->mo_toTexture;
    Particle_PrepareTexture(&to, (ParticleBlendType)m_blendType);
    for (INDEX i = 0; i < numParticles; ++i) {
      renderParticles[i]->Render(this);
    }
    Particle_Flush();
  }

  CPlacement3D GetLerpedPlacement() const
  {
    return CEntity::GetLerpedPlacement();
  }

  void MirrorAndStretch(FLOAT fStretch, BOOL bMirrorX)
  {
    m_sizeX *= fStretch;
    m_sizeY *= fStretch;
    m_maxsizeX *= fStretch;
    m_maxsizeY *= fStretch;
    m_fStretchX *= fStretch;
    m_fStretchY *= fStretch;
    m_fStretchZ *= fStretch;
  }
  
  void StretchModel()
  {
    if (Abs(m_fStretchX)  < 0.01f) { m_fStretchX   = 0.01f;  }
    if (Abs(m_fStretchY)  < 0.01f) { m_fStretchY   = 0.01f;  }
    if (Abs(m_fStretchZ)  < 0.01f) { m_fStretchZ   = 0.01f;  }

    if (Abs(m_fStretchX)  >100.0f) { m_fStretchX   = 100.0f*Sgn(m_fStretchX); }
    if (Abs(m_fStretchY)  >100.0f) { m_fStretchY   = 100.0f*Sgn(m_fStretchY); }
    if (Abs(m_fStretchZ)  >100.0f) { m_fStretchZ   = 100.0f*Sgn(m_fStretchZ); }

    GetModelObject()->StretchModel(FLOAT3D(m_fStretchX, m_fStretchY, m_fStretchZ));
    ModelChangeNotify();
  }

  void AdjustProperties()
  {
    if (!IsTargetValid(offsetof(ParametricParticles, m_penHeightMap), m_penHeightMap.ep_pen)) {
      m_penHeightMap = NULL;
    }
    if (!IsTargetValid(offsetof(ParametricParticles, m_penVelocity), m_penVelocity.ep_pen)) {
      m_penVelocity = NULL;
    }
    if (!IsTargetValid(offsetof(ParametricParticles, m_penRotation), m_penRotation.ep_pen)) {
      m_penRotation = NULL;
    }
    if (!IsTargetValid(offsetof(ParametricParticles, m_penSpawnerShape), m_penSpawnerShape.ep_pen)) {
      m_penSpawnerShape = NULL;
    }
    m_fSpawnInterval = ClampDn(m_fSpawnInterval, CTimer::TickQuantum);
    m_iSpawnMin = ClampDn(m_iSpawnMin, INDEX(0));
    m_iSpawnMax = ClampDn(m_iSpawnMax, m_iSpawnMin);
    m_fParticleLifetimeMin = ClampDn(m_fParticleLifetimeMin, CTimer::TickQuantum);
    m_fParticleLifetimeMax = ClampDn(m_fParticleLifetimeMax, m_fParticleLifetimeMin);
    m_iTextureNumRows = ClampDn(m_iTextureNumRows, INDEX(1));
    m_iTextureNumCols = ClampDn(m_iTextureNumCols, INDEX(1));
    m_sizeX = ClampDn(m_sizeX, 0.001f);
    m_sizeY = ClampDn(m_sizeY, 0.001f);
    m_maxsizeX = ClampDn(m_maxsizeX, m_sizeX);
    m_maxsizeY = ClampDn(m_maxsizeY, m_sizeY);
    m_textureSpeed = ClampDn(m_textureSpeed, 0.001f);
    m_textureTiles = ClampDn(m_textureTiles, INDEX(1));
    m_rotationMin = Clamp(m_rotationMin, -180.0f, 180.0f);
    m_rotationMax = Clamp(m_rotationMax, m_rotationMin, 180.0f);
    m_maxParticles = ClampDn(m_maxParticles, INDEX(1));
    m_particleFallOff = ClampDn(m_particleFallOff, 1.0f);
    m_particleHotSpot = Clamp(m_particleHotSpot, 0.0f, m_particleFallOff);
    m_updateStep = CTimer::TickQuantum;
    m_presimulationSpan = Clamp(m_presimulationSpan, 0.0f, 60.0f);
  }

  void KillParticles()
  {
    for (INDEX i = 0; i < m_maxParticles; ++i) {
      particles[i].birthTime = -1;
      particles[i].deathTime = -1;
    }
  }

  void Presimulate()
  {
    const ULONG prevRandom = _pNetwork->ga_sesSessionState.ses_ulRandomSeed;

    m_fLastSpawnTime = _pTimer->CurrentTick() - m_fSpawnInterval * 2.0f;
    const INDEX ticksToSimulate = floor(m_presimulationSpan / CTimer::TickQuantum);
    for (INDEX i = 0; i < ticksToSimulate; ++i)
    {
      UpdateParticles();
      m_fLastSpawnTime -= CTimer::TickQuantum;
      Particle* particle = lastUsed;
      while (particle)
      {
        particle->birthTime -= CTimer::TickQuantum;
        particle->deathTime -= CTimer::TickQuantum;
        particle = particle->prev;
      }
      m_updateStep = CTimer::TickQuantum;
    }
    
    _pNetwork->ga_sesSessionState.ses_ulRandomSeed = prevRandom;
  }

  void SetPlacement_internal(const CPlacement3D& plNew, const FLOATmatrix3D& mRotation, BOOL bNear)
  {
    CRationalEntity_EnableWeakPointer::SetPlacement_internal(plNew, mRotation, bNear);
    if (InWED())
    {
      End();
      Initialize();
    }
  }

procedures:
  Active()
  {
    if (m_presimulated) {
      m_presimulated = FALSE;
    } else {
      m_fLastSpawnTime = _pTimer->CurrentTick() - m_fSpawnInterval * 2.0f;
    }
    m_updateStep = CTimer::TickQuantum;
    m_bActive = TRUE;
    UpdateParticles();

    while (TRUE)
    {
      wait (m_updateStep)
      {
        on (ETimer) :    
        {
          UpdateParticles();
          stop;
        }
        on (EActivate) :
        {
          m_updateStep = CTimer::TickQuantum;
          stop;
        }
        on (EDeactivate) :
        {
          jump InactiveDecaying(); 
        }
        on (EStop) :
        {
          KillParticles();
          jump Inactive();
        }
      }
    }
  }

  InactiveDecaying()
  {
    m_bActive = FALSE;
    while (HasAliveParticles())
    {
      wait (m_updateStep)
      {
        on (ETimer) :
        {
          UpdateParticles();
          stop;
        }
        on (EActivate) :
        {
          jump Active();
        }
        on (EStop) :
        {
          KillParticles();
          stop;
        }
      }
    }

    jump Inactive();
  }
  
  Inactive()
  {
    m_bActive = FALSE;
    wait()   
    {
      on (EBegin) : { resume; }
      on (EActivate) :
      {
        jump Active(); 
      }
    }
  }

  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    
    StretchModel();
    SetModel(BOX_MODEL);
    try {
      if (m_fnTexture.Length() > 0) {
        GetModelObject()->mo_toTexture.SetData_t(m_fnTexture);
      } else {
        GetModelObject()->mo_toTexture.SetData_t(CTString("Models\\Editor\\Teleport.tex"));
      }
      GetModelObject()->mo_toTexture.PlayAnim(m_textureAnimation, AOF_LOOPING);
    } catch (const char* error) {
      WarningMessage(error);
    }
    try {
      m_colorAnimationObject.SetData_t(m_colorAnimationFile);
    } catch (const char* error) {
      m_colorAnimationFile = "";
      WarningMessage(error);
    }
    ModelChangeNotify();

    if (m_background) {
      SetFlags(GetFlags() | ENF_BACKGROUND);
    } else {
      SetFlags(GetFlags() & ~ENF_BACKGROUND);
    }

    AdjustProperties();

    EditGraphVariable(this, m_editAlpha, m_alphaGraph, LIMIT_GRAPH_X | LIMIT_GRAPH_Y);
    EditGraphVariable(this, m_editStretchX, m_stretchXGraph, LIMIT_GRAPH_X);
    EditGraphVariable(this, m_editStretchY, m_stretchYGraph, LIMIT_GRAPH_X);
    RecacheArrays();

    if (m_help) {
      m_help = FALSE;
      ShellExecute(NULL, NULL, "https://github.com/SeriousAlexej/ParametricParticles", NULL, NULL, SW_SHOW);
    }

    if (m_bActive) {
      Presimulate();
    }

    if (InWED())
    {
      if (m_penHeightMap) {
        ((AutoHeightMap*)m_penHeightMap.ep_pen)->p_parent = this;
      }
      if (m_penVelocity) {
        ((ParticleVelocity*)m_penVelocity.ep_pen)->p_parent = this;
      }
      if (m_penRotation) {
        ((ParticleRotation*)m_penRotation.ep_pen)->p_parent = this;
      }
      if (m_penSpawnerShape) {
        ((SpawnShapeBase*)m_penSpawnerShape.ep_pen)->p_parent = this;
      }
    }
    
    autowait(0.1f);

    m_presimulated = HasAliveParticles();

    if (m_bActive) {
      jump Active();
    } else {
      jump Inactive();
    }
  }
};
