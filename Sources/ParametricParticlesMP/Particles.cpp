#include "StdH.h"
#include "Particles.h"
#include "AutoHeightMap.h"
#include "ParametricParticles.h"
#include "SpawnShapeBox.h"
#include "SpawnShapeSphere.h"
#include "SpawnShapeCylinder.h"
#include "ParticleRotation.h"
#include "ParticleVelocity.h"

Particle::Particle()
: next(NULL)
, prev(NULL)
, position(0, 0, 0)
, lastPosition(0, 0, 0)
, tileRow(0)
, tileCol(0)
, birthTime(-1)
, deathTime(-1)
, additionalRotation(0)
{
}

FLOAT Particle::RandomFloat(ULONG& rndSeed) const
{
  rndSeed = rndSeed*262147;
  return ((rndSeed>>(31-24))&0xFFFFFF)/FLOAT(0xFFFFFF);
}

BOOL Particle::IsAlive() const
{
  return (_pTimer->CurrentTick() < deathTime) ? TRUE : FALSE;
}

BOOL Particle::IsVisible(const ParametricParticles* parent) const
{
  FLOAT3D pos = LerpedPos();
  if (parent->m_particlePlacement == PP_RELATIVE)
    pos = pos * g_parentLerpedRotationForComparison + g_parentLerpedPositionForComparison;

  const FLOAT distanceToViewer = (pos - g_viewerLerpedPositionForComparison).Length();
  return distanceToViewer < parent->m_particleFallOff ? TRUE : FALSE;
}

void Particle::Update(const ParametricParticles* parent)
{
  lastPosition = position;
  lastRotation = rotation;

  ULONG rndSeed = savedRndSeed;

  ParticleVelocity* velocity = (ParticleVelocity*)parent->m_penVelocity.ep_pen;
  while (velocity)
  {
    if (RandomFloat(rndSeed) <= velocity->m_probability)
    {
      FLOATmatrix3D velocityOrientation;
      MakeRotationMatrixFast(velocityOrientation, velocity->GetRandomAngles(RandomFloat(rndSeed), RandomFloat(rndSeed)));
      const FLOAT3D velDirection = FLOAT3D(0, 0, -1) * velocityOrientation * velocity->GetRotationMatrix();

      position += velDirection * velocity->GetVelocityStretchSince(birthTime) * parent->m_updateStep;
    }
    velocity = (ParticleVelocity*)velocity->m_penNext.ep_pen;
  }

  ParticleRotation* rotationp = (ParticleRotation*)parent->m_penRotation.ep_pen;
  while (rotationp)
  {
    if (RandomFloat(rndSeed) <= rotationp->m_probability)
    {
      const FLOAT rotSpeed = rotationp->m_rotationSpeedMin + RandomFloat(rndSeed) * (rotationp->m_rotationSpeedMax - rotationp->m_rotationSpeedMin);
      rotation += rotSpeed * rotationp->GetSpeedStretchSince(birthTime) * parent->m_updateStep;
    }
    rotationp = (ParticleRotation*)rotationp->m_penNext.ep_pen;
  }
}

void Particle::Create(ParametricParticles* parent)
{
  id = parent->m_freeID++;

  const FLOAT rndX = parent->FRnd();
  FLOAT rndY = rndX;
  if (!parent->m_sizeUniform)
    rndY = parent->FRnd();
  size(1) = parent->m_sizeX + rndX * (parent->m_maxsizeX - parent->m_sizeX);
  size(2) = parent->m_sizeY + rndY * (parent->m_maxsizeY - parent->m_sizeY);

  rotation = parent->m_rotationMin + parent->FRnd() * (parent->m_rotationMax - parent->m_rotationMin);
  lastRotation = rotation;
  additionalRotation = 0.0f;

  position = parent->GenerateSpawnPosition();
  lastPosition = position;

  if (parent->m_textureType == TT_RANDOM_FRAME) {
    tileRow = parent->IRnd() % parent->m_iTextureNumRows;
    tileCol = parent->IRnd() % parent->m_iTextureNumCols;
  } else {
    tileRow = 0;
    tileCol = 0;
  }
  birthTime = _pTimer->CurrentTick();
  const FLOAT lifetimeRndOffset = parent->m_fParticleLifetimeMax - parent->m_fParticleLifetimeMin;
  deathTime = birthTime + parent->m_fParticleLifetimeMin + parent->FRnd() * lifetimeRndOffset;

  savedRndSeed = _pNetwork->ga_sesSessionState.ses_ulRandomSeed;

  Update(parent);
}

void Particle::Render(ParametricParticles* parent) const
{
  FLOAT3D pos = LerpedPos();
  if (parent->m_particlePlacement == PP_RELATIVE)
    pos = pos * g_parentLerpedRotationForComparison + g_parentLerpedPositionForComparison;

  FLOAT baseAlpha = 1.0f;
  const FLOAT distanceToViewer = (pos - g_viewerLerpedPositionForComparison).Length();
  if (distanceToViewer > parent->m_particleHotSpot)
    baseAlpha = (parent->m_particleFallOff - distanceToViewer) / (parent->m_particleFallOff - parent->m_particleHotSpot);

  const FLOAT lerpedTick = _pTimer->GetLerpedCurrentTick();
  const FLOAT relativeLifetime = (lerpedTick - birthTime) / (deathTime - birthTime);
  const CTextureObject& to = parent->GetModelObject()->mo_toTexture;

  INDEX curCol = tileCol;
  INDEX curRow = tileRow;

  if (parent->m_textureType == TT_ANIMATED)
  {
    const TIME currAnimTime = lerpedTick - birthTime;
    const INDEX currFrame = INDEX(floor(currAnimTime * parent->m_textureSpeed)) % parent->m_textureTiles;
    curCol = currFrame % parent->m_iTextureNumCols;
    curRow = currFrame / parent->m_iTextureNumCols;
  }

  Particle_SetTexturePart(
    to.GetWidth() / parent->m_iTextureNumCols,
    to.GetHeight() / parent->m_iTextureNumRows,
    curCol,
    curRow);

  const CProjection3D* proj = Particle_GetProjection();
  FLOAT3D xAxis;
  FLOAT3D yAxis;
  if (parent->m_flatType == FLAT_HALF)
  {
    const FLOAT3D zAxis = proj->pr_ViewerRotationMatrix.GetRow(3);
    yAxis = (!parent->GetRotationMatrix()).GetRow(2);
    const FLOAT3D yAxisView =
      proj->pr_ViewerRotationMatrix.GetRow(2)*(yAxis%proj->pr_ViewerRotationMatrix.GetRow(2)) +
      proj->pr_ViewerRotationMatrix.GetRow(1)*(yAxis%proj->pr_ViewerRotationMatrix.GetRow(1));
    xAxis = yAxisView*zAxis;
    const FLOAT xLen = xAxis.Length();
    if (xLen < 1e-6) {
      xAxis = proj->pr_ViewerRotationMatrix.GetRow(1);
    } else {
      xAxis /= xLen;
    }
  } else if (parent->m_flatType == FLAT_FULL) {
    xAxis = proj->pr_ViewerRotationMatrix.GetRow(1);
    yAxis = proj->pr_ViewerRotationMatrix.GetRow(2);
  } else {
    const FLOATmatrix3D axes = !g_parentLerpedRotationForComparison;
    xAxis = axes.GetRow(1);
    yAxis = axes.GetRow(2);
  }
  if (parent->m_followVelocity)
  {
    const FLOAT3D zAxis = xAxis * yAxis;
    FLOATmatrix3D basis;
    basis.matrix[0][0] = xAxis(1);
    basis.matrix[0][1] = xAxis(2);
    basis.matrix[0][2] = xAxis(3);
    basis.matrix[1][0] = yAxis(1);
    basis.matrix[1][1] = yAxis(2);
    basis.matrix[1][2] = yAxis(3);
    basis.matrix[2][0] = zAxis(1);
    basis.matrix[2][1] = zAxis(2);
    basis.matrix[2][2] = zAxis(3);
    FLOAT3D dir = position - lastPosition;
    if (parent->m_particlePlacement == PP_RELATIVE)
      dir *= g_parentLerpedRotationForComparison;
    dir *= basis;
    const FLOAT2D dirProj(dir(1), dir(2));
    const FLOAT len = dirProj.Length();
    if (len > 0.00001f)
    {
      const FLOAT rotRad = additionalRotation * PI / 180.0f;
      const FLOAT2D c(-sin(rotRad), cos(rotRad));
      FLOAT2D desired = (dirProj/len + c) * 0.5f;
      if (Abs(desired(1)) < 0.001f && Abs(desired(2)) < 0.001f)
      {
        additionalRotation += 90.0f;
      } else {
        desired.SafeNormalize();
        FLOAT a = acos(Clamp(desired(2), -1.0f, 1.0f));
        if (desired(1) > 0.0f)
          a *= -1.0f;
        additionalRotation = a * 180.0f / PI;
      }
    }
  }
  const FLOAT rot = LerpedRot() - additionalRotation;
  if (rot != 0.0f)
  {
    const FLOAT rotRad = rot * PI / 180.0f;
    const FLOAT sinRot = sin(rotRad);
    const FLOAT cosRot = cos(rotRad);
    const FLOAT3D xAxisR = xAxis*cosRot - yAxis*sinRot;
    const FLOAT3D yAxisR = xAxis*sinRot + yAxis*cosRot;
    xAxis = xAxisR;
    yAxis = yAxisR;
  }

  FLOAT3D v0, v1, v2, v3;
  const FLOAT width = size(1) * parent->GetXStretch(relativeLifetime);
  const FLOAT height = size(2) * parent->GetYStretch(relativeLifetime);
  v0 = pos - xAxis*width*0.5f + yAxis*height*0.5f;
  v1 = pos - xAxis*width*0.5f - yAxis*height*0.5f;
  v2 = pos + xAxis*width*0.5f - yAxis*height*0.5f;
  v3 = pos + xAxis*width*0.5f + yAxis*height*0.5f;

  Particle_RenderQuad3D(v0, v1, v2, v3, parent->GetColorSince(birthTime)|NormFloatToByte(baseAlpha * parent->GetAlpha(relativeLifetime, pos)));
}

void Particle::Write(CTStream* strm) const
{
  CTStream& s = *strm;
  const INDEX versionMajor = 0;
  const INDEX versionMinor = 0;
  s << versionMajor;
  s << versionMinor;
  s << id;
  s << position(1) << position(2) << position(3);
  s << lastPosition(1) << lastPosition(2) << lastPosition(3);
  s << size(1) << size(2);
  s << rotation;
  s << lastRotation;
  s << tileRow;
  s << tileCol;
  s << birthTime;
  s << deathTime;
  s << savedRndSeed;
}

void Particle::Read(CTStream* strm)
{
  next = NULL;
  prev = NULL;
  additionalRotation = 0.0f;
  CTStream& s = *strm;
  INDEX versionMajor;
  INDEX versionMinor;
  s >> versionMajor;
  s >> versionMinor;
  s >> id;
  s >> position(1) >> position(2) >> position(3);
  s >> lastPosition(1) >> lastPosition(2) >> lastPosition(3);
  s >> size(1) >> size(2);
  s >> rotation;
  s >> lastRotation;
  s >> tileRow;
  s >> tileCol;
  s >> birthTime;
  s >> deathTime;
  s >> savedRndSeed;
}

FLOAT3D Particle::LerpedPos() const
{
  return Lerp(lastPosition, position, g_parentIsPredictor ? _pTimer->GetLerpFactor() : _pTimer->GetLerpFactor2());
}

FLOAT Particle::LerpedRot() const
{
  return Lerp(lastRotation, rotation, g_parentIsPredictor ? _pTimer->GetLerpFactor() : _pTimer->GetLerpFactor2());
}

int Particle::CompareID(const void* lhs, const void* rhs)
{
  const Particle& particleLhs = **(const Particle**)lhs;
  const Particle& particleRhs = **(const Particle**)rhs;

  if (particleLhs.id < particleRhs.id)
    return -1;
  if (particleLhs.id > particleRhs.id)
    return 1;
  return 0;
}

int Particle::CompareDistance(const void* lhs, const void* rhs)
{
  const Particle& particleLhs = **(const Particle**)lhs;
  const Particle& particleRhs = **(const Particle**)rhs;

  FLOAT3D posLhs = particleLhs.LerpedPos();
  FLOAT3D posRhs = particleRhs.LerpedPos();
  if (g_parentRelativePlacement)
  {
    posLhs = posLhs * g_parentLerpedRotationForComparison + g_parentLerpedPositionForComparison;
    posRhs = posRhs * g_parentLerpedRotationForComparison + g_parentLerpedPositionForComparison;
  }

  const FLOAT distLhs = (g_viewerLerpedPositionForComparison - posLhs)%(g_projectionZAxis);
  const FLOAT distRhs = (g_viewerLerpedPositionForComparison - posRhs)%(g_projectionZAxis);
  
  return FloatToInt((distRhs - distLhs) * 1000);
}

void ReinitParent(WeakPointer& p_parent, CEntity* self)
{
  CEntity* parent = p_parent.Get();
  if (!InWED() || !parent)
    return;

  bool ok = true;
  switch (ENTITY_ID(parent))
  {
  case ID_ParametricParticles:
    switch (ENTITY_ID(self))
    {
    case ID_ParticleRotation:
      ok = ((ParametricParticles*)parent)->m_penRotation.ep_pen == self;
      break;
    case ID_ParticleVelocity:
      ok = ((ParametricParticles*)parent)->m_penVelocity.ep_pen == self;
      break;
    case ID_AutoHeightMap:
      ok = ((ParametricParticles*)parent)->m_penHeightMap.ep_pen == self;
      break;
    default:
      ok = ((ParametricParticles*)parent)->m_penSpawnerShape.ep_pen == self;
      break;
    }
    break;
  case ID_ParticleRotation:
    ok = ((ParticleRotation*)parent)->m_penNext.ep_pen == self;
    break;
  case ID_ParticleVelocity:
    ok = ((ParticleVelocity*)parent)->m_penNext.ep_pen == self;
    break;
  case ID_AutoHeightMap:
    ok = ((AutoHeightMap*)parent)->m_penNext.ep_pen == self;
    break;
  default: break;
  }

  if (ok)
  {
    parent->End();
    parent->Initialize();
  } else {
    p_parent = NULL;
  }
}

extern BOOL g_parentIsPredictor = TRUE;
extern BOOL g_parentRelativePlacement = FALSE;
extern FLOAT3D g_projectionZAxis(0, 0, -1);
extern FLOAT3D g_viewerLerpedPositionForComparison(0, 0, 0);
extern FLOAT3D g_parentLerpedPositionForComparison(0, 0, 0);
extern FLOATmatrix3D g_parentLerpedRotationForComparison = FLOATmatrix3D();
