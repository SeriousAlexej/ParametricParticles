4242
%{
#include "StdH.h"
#include "Particles.h"
#define ID_PARAMETRIC_PARTICLES "PPAR"
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

class ParametricParticles : CMovableModelEntity
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
 27 COLOR m_color "Particle color" = C_WHITE,
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

{
  Particle* lastFree;
  Particle* lastUsed;
  Particle particles_MAX_PARTICLES;
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
    }
    return CMovableModelEntity::GetAnimData(slPropertyOffset);
  }

  void RecacheGraphs()
  {
    RecacheGraph(alphaCache, m_alphaGraph);
    RecacheGraph(stretchXCache, m_stretchXGraph);
    RecacheGraph(stretchYCache, m_stretchYGraph);
  }

  FLOAT GetAlpha(FLOAT f) const
  {
    return Clamp(GetGraphValueAt(alphaCache, f), 0.0f, 1.0f);
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
    for (INDEX i = 0; i < MAX_PARTICLES; ++i) {
      PushFree(&particles[i]);
    }
  }
  
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity* penTarget)
  {
    if (slPropertyOffset == offsetof(ParametricParticles, m_penVelocity))
    {
      if (!penTarget) {
        return TRUE;
      }
      return (penTarget->GetClass()->ec_pdecDLLClass->dec_iID == 4247) ? TRUE: FALSE;
    }

    if (slPropertyOffset == offsetof(ParametricParticles, m_penRotation))
    {
      if (!penTarget) {
        return TRUE;
      }
      return (penTarget->GetClass()->ec_pdecDLLClass->dec_iID == 4246) ? TRUE : FALSE;
    }
    
    if (slPropertyOffset == offsetof(ParametricParticles, m_penSpawnerShape))
    {
      if (!penTarget) {
        return TRUE;
      }
      switch (penTarget->GetClass()->ec_pdecDLLClass->dec_iID)
      {
      case 4243:
      case 4244:
      case 4245:
        return TRUE;
      }
      return FALSE;
    }

    return CMovableModelEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  void Read_t(CTStream* strm)
  { 
    CMovableModelEntity::Read_t(strm);
    RecacheGraphs();
    
    strm->ExpectID_t(ID_PARAMETRIC_PARTICLES);

    INDEX numActive = 0;
    (*strm) >> numActive;

    lastUsed = NULL;
    if (numActive > 0) {
      for (INDEX i = 0; i < numActive; ++i) {
        particles[i].Read(strm);
      }
      particles[0].prev = NULL;
      for (i = 1; i < numActive; ++i) {
        Particle& lhs = particles[i - 1];
        Particle& rhs = particles[i];
        lhs.next = &rhs;
        rhs.prev = &lhs;
      }
      particles[numActive - 1].next = NULL;
      lastUsed = &particles[numActive - 1];
    }

    lastFree = NULL;
    for (INDEX i = numActive; i < MAX_PARTICLES; ++i) {
      particles[i].prev = NULL;
      particles[i].next = NULL;
    }
    for (i = numActive; i < MAX_PARTICLES; ++i) {
      PushFree(&particles[i]);
    }
  }
  
  void Write_t(CTStream* strm)
  {
    CMovableModelEntity::Write_t(strm);
    
    strm->WriteID_t(CChunkID(ID_PARAMETRIC_PARTICLES));

    Particle* activeParticles[MAX_PARTICLES];
    INDEX numActive = 0;
    Particle* particle = lastUsed;
    while (particle) {
      activeParticles[numActive++] = particle;
      particle = particle->prev;
    }
    qsort(activeParticles, numActive, sizeof(Particle*), &Particle::CompareID);
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
    Particle* particle = lastUsed;
    while (particle)
    {
      Particle* current = particle;
      particle = current->prev;

      if (current->IsAlive()) {
        current->Update(this);
      } else {
        PushFree(current);
      }
    }

    if (!m_bActive || m_fLastSpawnTime + m_fSpawnInterval > _pTimer->CurrentTick()) {
      return;
    }

    m_fLastSpawnTime = _pTimer->CurrentTick();
    const INDEX numToSpawn = m_iSpawnMin + (IRnd() % (m_iSpawnMax - m_iSpawnMin + 1));
    for (INDEX i = 0; i < numToSpawn; ++i)
    {
      Particle* newParticle = PopFree();
      if (!newParticle) {
        return;
      }
      newParticle->Create(this);
    }
  }

  void RenderParticles()
  {
    if (!HasAliveParticles()) {
      return;
    }
    const CProjection3D* proj = Particle_GetProjection();
    const CEntity* viewer = Particle_GetViewer();
    if (!viewer || !proj) {
      return;
    }

    static Particle* renderParticles[MAX_PARTICLES];
    INDEX numParticles = 0;

    Particle* particle = lastUsed;
    while (particle)
    {
      renderParticles[numParticles++] = particle;
      particle = particle->prev;
    }
    g_parentIsPredictor = IsPredictor();
    g_parentRelativePlacement = m_particlePlacement == PP_RELATIVE ? TRUE : FALSE;
    g_projectionZAxis = Particle_GetProjection()->pr_ViewerRotationMatrix.GetRow(3);
    const CPlacement3D lerpedPlacement = GetLerpedPlacement();
    g_parentLerpedPositionForComparison = lerpedPlacement.pl_PositionVector;
    MakeRotationMatrixFast(g_parentLerpedRotationForComparison, lerpedPlacement.pl_OrientationAngle);
    g_viewerLerpedPositionForComparison = Particle_GetViewer()->GetLerpedPlacement().pl_PositionVector;
    qsort(renderParticles, numParticles, sizeof(Particle*), &Particle::CompareDistance);

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
    m_fStretchX *= fStretch;
    m_fStretchY *= fStretch;
    m_fStretchZ *= fStretch;

    if (bMirrorX) {
      m_fStretchX = -m_fStretchX;
    }
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
  }

  void KillParticles()
  {
    for (INDEX i = 0; i < MAX_PARTICLES; ++i) {
      particles[i].birthTime = -1;
      particles[i].deathTime = -1;
    }
  }

procedures:
  Active()
  {
    m_bActive = TRUE;
    m_fLastSpawnTime = _pTimer->CurrentTick() - m_fSpawnInterval * 2.0f;

    while (TRUE)
    {
      wait (CTimer::TickQuantum)
      {
        on (EBegin) :
        {
          UpdateParticles();
          resume;
        }
        on (ETimer) : { stop; }
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
      wait (CTimer::TickQuantum)
      {
        on (EBegin) :
        {
          UpdateParticles();
          resume;
        }
        on (ETimer) : { stop; }
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
      GetModelObject()->mo_toTexture.SetData_t(m_fnTexture);
      GetModelObject()->mo_toTexture.PlayAnim(m_textureAnimation, AOF_LOOPING);
    } catch (const char* error) {
      WarningMessage(error);
    }
    ModelChangeNotify();

    if (m_background) {
      SetFlags(GetFlags() | ENF_BACKGROUND);
    } else {
      SetFlags(GetFlags() & ~ENF_BACKGROUND);
    }
    
    // apparently some conditional hack with staying in
    // list of movers - taken from default ParticlesHolder
    en_fGravityA = 30.0f;
    GetPitchDirection(-90.0f, en_vGravityDir);

    AdjustProperties();

    EditGraphVariable(this, m_editAlpha, m_alphaGraph, LIMIT_GRAPH_X | LIMIT_GRAPH_Y);
    EditGraphVariable(this, m_editStretchX, m_stretchXGraph, LIMIT_GRAPH_X);
    EditGraphVariable(this, m_editStretchY, m_stretchYGraph, LIMIT_GRAPH_X);
    RecacheGraphs();

    if (m_help) {
      m_help = FALSE;
      ShellExecute(NULL, NULL, "https://github.com/SeriousAlexej/ParametricParticles", NULL, NULL, SW_SHOW);
    }
    
    autowait(0.1f);

    if (m_bActive) {
      jump Active();
    } else {
      jump Inactive();
    }
  }
};
