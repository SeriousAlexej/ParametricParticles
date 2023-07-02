#include "StdH.h"
#include "Particles.h"
#include "AutoHeightMap.h"
#include "ParametricParticles.h"
#include "SpawnShapeBox.h"
#include "SpawnShapeSphere.h"
#include "SpawnShapeCylinder.h"
#include "ParticleRotation.h"
#include "ParticleVelocity.h"

FLOAT GetDiscreteGraphValueSince(const CStaticArray<FLOAT>& graph, FLOAT birthTime, BOOL loop)
{
  if (graph.Count() == 0)
    return 1.0f;

  INDEX graphIndex = ClampDn(INDEX(floor(0.5f + (_pTimer->CurrentTick() - birthTime) / CTimer::TickQuantum)), INDEX(0));
  if (loop)
    graphIndex %= graph.Count();
  else
    graphIndex = ClampUp(graphIndex, INDEX(graph.Count() - 1));

  return graph[graphIndex];
}

FLOAT GetGraphValueAt(const CStaticArray<FLOAT2D>& graph, FLOAT f)
{
  if (graph.Count() == 0)
    return 1.0f;

  INDEX i = LowerBound(graph, f);
  if (i >= graph.Count()) {
    return graph[graph.Count()-1](2);
  }
  if (i == 0) {
    return graph[0](2);
  }
  const FLOAT2D& a = graph[i-1];
  const FLOAT2D& b = graph[i];
  const FLOAT af = a(1);
  const FLOAT bf = b(1);
  const FLOAT pf = (f - af) / (bf - af);
  return Lerp(a(2), b(2), pf);
}

void RecacheGraph(CStaticArray<FLOAT2D>& cache, const CTString& graph)
{
  cache.Clear();
  INDEX numCoords = 0;
  if (graph.Length() <= 0) {
    return;
  }
  INDEX offset = 0;
  INDEX pos = 0;
  FLOAT dummy;
  while (sscanf(graph.str_String + offset, "%f %f%n", &dummy, &dummy, &pos) == 2) {
    offset += pos;
    ++numCoords;
  }

  if (numCoords == 0) {
    return;
  }
  offset = 0;
  pos = 0;
  cache.New(numCoords);
  for (INDEX i = 0; i < numCoords; ++i) {
    sscanf(graph.str_String + offset, "%f %f%n", &cache[i](1), &cache[i](2), &pos);
    offset += pos;
  }
}

void RecacheGraphDiscrete(CStaticArray<FLOAT>& cache, const CTString& graphStr)
{
  cache.Clear();
  CStaticArray<FLOAT2D> graph;
  RecacheGraph(graph, graphStr);
  if (graph.Count() == 0)
    return;

  const FLOAT maxTime = graph[graph.Count() - 1](1);
  if (maxTime <= CTimer::TickQuantum)
  {
    cache.New(1);
    cache[0] = graph[graph.Count() - 1](2);
    return;
  }

  cache.New(floor(maxTime / CTimer::TickQuantum + 0.5f) + 1);
  for (INDEX i = 0; i < cache.Count(); ++i)
  {
    const TIME currTime = (i * CTimer::TickQuantum);
    cache[i] = GetGraphValueAt(graph, currTime);
  }
}

INDEX LowerBound(const CStaticArray<FLOAT2D>& arr, const FLOAT value)
{
  INDEX count = arr.Count();
  INDEX step = 0;
  INDEX first = 0;
  INDEX it = 0;

  while (count > 0)
  {
    it = first; 
    step = count / 2; 
    it += step;

    if (arr[it](1) < value)
    {
      first = ++it; 
      count -= step + 1; 
    }
    else
    {
      count = step;
    }
  }

  return first;
}

void EditGraphVariable(const CEntity* self, BOOL& property, CTString& graph, ULONG flags)
{
  if (!property)
    return;
  property = FALSE;

  const CTString graphExe = _fnmApplicationPath + "Bin\\Graph.exe";
  const CTString tmpGraphFile = "Temp\\graph.txt";
  CTString propertyName = "";

  const SLONG offset = ((const char*)&property) - ((const char*)self);
  const CDLLEntityClass* dllClass = self->en_pecClass->ec_pdecDLLClass;
  for (INDEX i = 0; i < dllClass->dec_ctProperties; ++i)
  {
    const CEntityProperty& prop = dllClass->dec_aepProperties[i];
    if (prop.ep_slOffset == offset)
    {
      propertyName = prop.ep_strName;
      break;
    }
  }

  FILE* graphFile = fopen((_fnmApplicationPath + tmpGraphFile).str_String, "w");
  if (!graphFile)
  {
    WarningMessage("Failed to write into temp file %s!", tmpGraphFile.str_String);
    return;
  }
  fprintf(graphFile, "%d\n", flags);
  fprintf(graphFile, "%s\n", propertyName.str_String);
  fprintf(graphFile, graph.str_String);
  fclose(graphFile);

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  if (!CreateProcess(graphExe.str_String, tmpGraphFile.str_String, NULL, NULL, FALSE, 0, NULL, _fnmApplicationPath.str_String, &si, &pi))
  {
    WarningMessage("Failed to launch Graph.exe! Repair your installation.");
    return;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exitCode = 0;
  if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode == 0)
  {
    graphFile = fopen((_fnmApplicationPath + tmpGraphFile).str_String, "r");
    if (!graphFile)
    {
      WarningMessage("Failed to read from temp file %s!", tmpGraphFile.str_String);
      return;
    }
    graph = "";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), graphFile))
      graph += buffer;
    fclose(graphFile);
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}

#ifdef USE_CUSTOM_PARTICLE_PROJECTION
CProjection3D* Particle_GetProjection()
{
  static const int enginedllBaseAddress = reinterpret_cast<int>(&CEntity::HandleSentEvents) - 0xFEC80;
  static CProjection3D** ppProjection = reinterpret_cast<CProjection3D**>(enginedllBaseAddress + 0x1F64C8);
  return *ppProjection;
}
#endif

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

  if (parent->m_penSpawnerShape)
    position = ((SpawnShapeBase*)&*parent->m_penSpawnerShape)->GeneratePosition();
  else
    position = parent->GetPlacement().pl_PositionVector;

  if (parent->m_particlePlacement == PP_RELATIVE)
    position = (position - parent->GetPlacement().pl_PositionVector) * (!parent->GetRotationMatrix());

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
