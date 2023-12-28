4299
%{
#include <Engine/Engine.h>
#define DECL_DLL __declspec(dllimport)
#include "EntitiesMP/Global.h"
#undef DECL_DLL
#define DECL_DLL __declspec(dllexport)
#include "ParticleDisasm.h"
#include "VertexUtils.h"
#include <ParametricParticlesUtils/ParticlesUtils.h>
#include <ClipUnion/ClipUnion.h>

#define OUTLINE_COUNT 10
#define outlineEntities_ARRAY_TEN outlineEntities[OUTLINE_COUNT]
#define MIN_EXTENSION 0.001f

template<typename T>
const T Cross(const Vector<T, 2>& v1, const Vector<T, 2>& v2)
{
  return v1(1) * v2(2) - v2(1) * v1(2);
}

template<typename T>
bool RightTurn(const Vector<T, 2>& v1, const Vector<T, 2>& v2)
{
  return Cross(v1, v2) < static_cast<T>(0);
}

template<typename T>
bool LeftTurn(const Vector<T, 2>& v1, const Vector<T, 2>& v2)
{
  return Cross(v1, v2) > static_cast<T>(0);
}

const PIX2D* g_pivot = NULL;
int CompareByPivotAngle(const void* lhs, const void* rhs)
{
  const PIX2D& v1 = (**(const GrahamNode**)lhs).pos;
  const PIX2D& v2 = (**(const GrahamNode**)rhs).pos;
  const PIX2D& pivot = *g_pivot;
  const PIX2D a = v1 - pivot;
  const PIX2D b = v2 - pivot;
  if (a(2) == 0 && a(1) > 0)
    return -1;
  if (b(2) == 0 && b(1) > 0)
    return 1;
  if (a(2) > 0 && b(2) < 0)
    return -1;
  if (a(2) < 0 && b(2) > 0)
    return 1;
  const FLOAT cross = Cross(a, b);
  if (cross > 0)
    return -1;
  if (cross < 0)
    return 1;
  return (a % a) < (b % b) ? -1 : 1;
}

void IntersectEdges(FLOAT2D& res, const FLOAT2D& p, const FLOAT2D& pr, const FLOAT2D& q, const FLOAT2D& qs)
{
  const FLOAT2D r = pr - p;
  const FLOAT2D s = qs - q;
  const FLOAT rXs = Cross(r, s);
  if (fabs(rXs) < 0.00001f)
    return;

  const FLOAT2D qMp = q-p;
  const FLOAT t = Cross(qMp, s) / rXs;
  const FLOAT u = Cross(qMp, r) / rXs;
  if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f)
    res = p + r*t;
}
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

enum eOutlineType
{
  0 OT_CONVEX_REAL_DEPTH "Convex (real depth)",
  1 OT_CONVEX_AVG_DEPTH "Convex (average depth)",
  2 OT_CONVEX_FIXED_DEPTH "Convex (fixed depth)",
  3 OT_CONTOUR_AVG_DEPTH "Contour (average depth, expensive)",
  4 OT_CONTOUR_FIXED_DEPTH "Contour (fixed depth, expensive)",
};

class Outline : CRationalEntity
{
  name "Outline";
  thumbnail "Thumbnails\\Outline.tbn";
  features "HasName", "IsTargetable";

properties:
  1 CEntityPointer m_glowEntity1 "Outline entity 01",
  2 CEntityPointer m_glowEntity2 "Outline entity 02",
  3 CEntityPointer m_glowEntity3 "Outline entity 03",
  4 CEntityPointer m_glowEntity4 "Outline entity 04",
  5 CEntityPointer m_glowEntity5 "Outline entity 05",
  6 CEntityPointer m_glowEntity6 "Outline entity 06",
  7 CEntityPointer m_glowEntity7 "Outline entity 07",
  8 CEntityPointer m_glowEntity8 "Outline entity 08",
  9 CEntityPointer m_glowEntity9 "Outline entity 09",
 10 CEntityPointer m_glowEntity10 "Outline entity 10",
 11 CEntityPointer m_glowEntityEnd,
 12 COLOR m_color "Color" = -1,
 13 FLOAT m_extension "Thickness" = 0.25f,
 14 CTFileName m_texture "Texture" 'T' = CTFILENAME("TexturesMP\\Detail\\White.tex"),
 15 BOOL m_continuousUVx "UV X Continuous" = FALSE,
 16 BOOL m_depthTesting "Depth testing" = TRUE,
 17 enum eOutlineType m_type "Type" = OT_CONVEX_REAL_DEPTH,
 18 enum eParticleBlendType m_blendType "Blend type" = ePBT_BLEND,
 19 CTString m_strName "Name" 'N' = "Outline",
 20 BOOL m_bActive "Active" 'A' = TRUE,
 21 BOOL m_background "Background" = FALSE,
 22 BOOL m_extensionScaleEdit "Thickness scale..." = FALSE,
 23 CTString m_extensionScale = "0 1",
 24 FLOAT m_activatedTime = 0.0f,
 25 BOOL m_alphaScaleEdit "Color alpha scale..." = FALSE,
 26 CTString m_alphaScale = "0 1",
 27 ANIMATION m_textureAnimation "Texture animation" = 0,
 28 CAnimObject m_colorAnimationObject,
 29 ANIMATION m_colorAnimation "Color animation" = 0,
 30 CTFileName m_colorAnimationFile "Color animation file" = CTString(""),
 31 RANGE m_fallOff "Outline visibility fall-off" = 30.0f,
 32 RANGE m_hotSpot "Outline visibility hot-spot" = 25.0f,
 33 BOOL m_uvXOffsetEdit "UV X offset..." = FALSE,
 34 CTString m_uvXOffset = "0 0",
 35 BOOL m_uvYOffsetEdit "UV Y offset..." = FALSE,
 36 CTString m_uvYOffset = "0 0",
 37 FLOAT m_averageDepthOffset "Depth offset" = 0.0f,
 38 FLOAT m_UVAspect "UV Aspect ratio" = 1.0f,
 39 BOOL m_help "Online Help..." = FALSE,

{
  EntityAndPlacement outlineEntities_ARRAY_TEN;
  FLOAT3D outlineAverageCenter;
  CStaticArray<FLOAT> extensionScale;
  CStaticArray<FLOAT> alphaScale;
  CStaticArray<FLOAT> uvXOffsetGraph;
  CStaticArray<FLOAT> uvYOffsetGraph;
  FLOAT extension3D;
  FLOAT uvAspect;
  FLOAT uvXOffset;
  FLOAT uvYOffset;
}

components:
  1 model MODEL_OUTLINE "Models\\Editor\\Outline.mdl"

functions:
  //----------------------------------------------------------------------------------------------------
  CAnimData* GetAnimData(SLONG slPropertyOffset)
  {
    if (slPropertyOffset == offsetof(Outline, m_textureAnimation)) {
      return GetModelObject()->mo_toTexture.GetData();
    } else if (slPropertyOffset == offsetof(Outline, m_colorAnimation)) {
      return m_colorAnimationObject.GetData();
    }
    return CRationalEntity::GetAnimData(slPropertyOffset);
  }

  //----------------------------------------------------------------------------------------------------
  void RecacheGraphs()
  {
    RecacheGraphDiscrete(extensionScale, m_extensionScale);
    RecacheGraphDiscrete(alphaScale, m_alphaScale);
    RecacheGraphDiscrete(uvXOffsetGraph, m_uvXOffset);
    RecacheGraphDiscrete(uvYOffsetGraph, m_uvYOffset);
  }
  
  //----------------------------------------------------------------------------------------------------
  void Read_t(CTStream* strm)
  {
    CRationalEntity::Read_t(strm);
    RecacheGraphs();
  }
  
  //----------------------------------------------------------------------------------------------------
  FLOAT GetDistanceMul(const CPerspectiveProjection3D& proj) const
  {
    const FLOAT3D viewerPos = ProjectCoordinateReverse(FLOAT3D(0, 0, 0), proj);
    const FLOAT distanceToViewer = (viewerPos - outlineAverageCenter).Length();
    if (distanceToViewer >= m_fallOff) {
      return 0.0f;
    }
    if (distanceToViewer > m_hotSpot) {
      return (m_fallOff - distanceToViewer) / (m_fallOff - m_hotSpot);
    }
    return 1.0f;
  }

  //----------------------------------------------------------------------------------------------------
  FLOAT GetUVXOffset() const
  {
    FLOAT f1 = GetDiscreteGraphValueSince(uvXOffsetGraph, m_activatedTime + CTimer::TickQuantum, TRUE);
    FLOAT f2 = GetDiscreteGraphValueSince(uvXOffsetGraph, m_activatedTime, TRUE);
    const INDEX cnt = uvXOffsetGraph.Count();
    if (cnt > 1)
    {
      const INDEX graphIndex = ClampDn(INDEX(floor(0.5f + (_pTimer->CurrentTick() - m_activatedTime) / CTimer::TickQuantum)), INDEX(0)) % cnt;
      if (graphIndex == 0)
      {
        f1 -= static_cast<INDEX>(f1);
        f2 -= static_cast<INDEX>(f2);
        if (f1 < 0.0f) {
          f1 += 1.0f;
        }
        if (f2 < 0.0f) {
          f2 += 1.0f;
        }
      }
    }
    return Lerp(f1, f2, _pTimer->GetLerpFactor());
  }

  //----------------------------------------------------------------------------------------------------
  FLOAT GetUVYOffset() const
  {
    FLOAT f1 = GetDiscreteGraphValueSince(uvYOffsetGraph, m_activatedTime + CTimer::TickQuantum, TRUE);
    FLOAT f2 = GetDiscreteGraphValueSince(uvYOffsetGraph, m_activatedTime, TRUE);
    const INDEX cnt = uvYOffsetGraph.Count();
    if (cnt > 1)
    {
      const INDEX graphIndex = ClampDn(INDEX(floor(0.5f + (_pTimer->CurrentTick() - m_activatedTime) / CTimer::TickQuantum)), INDEX(0)) % cnt;
      if (graphIndex == 0)
      {
        f1 -= static_cast<INDEX>(f1);
        f2 -= static_cast<INDEX>(f2);
        if (f1 < 0.0f) {
          f1 += 1.0f;
        }
        if (f2 < 0.0f) {
          f2 += 1.0f;
        }
      }
    }
    return Lerp(f1, f2, _pTimer->GetLerpFactor());
  }

  //----------------------------------------------------------------------------------------------------
  COLOR GetColor(const FLOAT distanceMul) const
  {
    const FLOAT a1 = GetDiscreteGraphValueSince(alphaScale, m_activatedTime + CTimer::TickQuantum, TRUE);
    const FLOAT a2 = GetDiscreteGraphValueSince(alphaScale, m_activatedTime, TRUE);
    const FLOAT alphaMul = Clamp(Lerp(a1, a2, _pTimer->GetLerpFactor()), 0.0f, 1.0f);

    COLOR col = m_color;
    const FLOAT baseAlpha = NormByteToFloat(col & 0xFF);

    const CAnimData* animData = m_colorAnimationObject.ao_AnimData;
    if (animData && m_colorAnimation < animData->ad_NumberOfAnims)
    {
      const COneAnim& anim = animData->ad_Anims[m_colorAnimation];
      if (anim.oa_NumberOfFrames > 0)
      {
        const TIME tmCurrentRelative = _pTimer->GetLerpedCurrentTick() - m_activatedTime;
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

    return (col & C_WHITE) | NormFloatToByte(baseAlpha * alphaMul * distanceMul);
  }

  //----------------------------------------------------------------------------------------------------
  FLOAT Get2DExtension(const FLOAT averageZ, const CPerspectiveProjection3D& proj) const
  {
    FLOAT3D v0 = ProjectCoordinateReverse(FLOAT3D(0.0f, 0.0f, averageZ), proj);
    FLOAT3D v1 = ProjectCoordinateReverse(FLOAT3D(100.0f, 0.0f, averageZ), proj);
    FLOAT3D norm = v1 - v0;
    norm *= extension3D / norm.Length();
    proj.ProjectCoordinate(v0 + norm, v1);
    return v1(1);
  }

  //----------------------------------------------------------------------------------------------------
  FLOAT Get3DExtension() const
  {
    const FLOAT s1 = GetDiscreteGraphValueSince(extensionScale, m_activatedTime + CTimer::TickQuantum, TRUE);
    const FLOAT s2 = GetDiscreteGraphValueSince(extensionScale, m_activatedTime, TRUE);
    return m_extension * Lerp(s1, s2, _pTimer->GetLerpFactor());
  }

  //----------------------------------------------------------------------------------------------------
  void RenderCorner(const FLOAT extension, const FLOAT3D& v0, const FLOAT3D& v1, const FLOAT3D& v2, FLOAT& uvX, const COLOR color) const
  {
    const FLOAT3D vleft = v1-v0;
    const FLOAT3D vright = v2-v0;
    const FLOAT angle = acos(Clamp((vleft % vright) / (extension * extension), -1.0f, 1.0f));
    const FLOAT segmentAngle = 0.17453f; // 10 degrees in radians
    if (angle >= segmentAngle * 2.0f)
    {
      const INDEX numStops = angle / segmentAngle;
      const FLOAT uvWidth = angle * uvAspect;
      const FLOAT uvIncrement = 2.0f * uvWidth / numStops;
      FLOAT3D normal = vleft * vright;
      normal.Normalize();
      FLOATquat3D rotationQuat;
      rotationQuat.FromAxisAngle(normal, angle / numStops);
      FLOATmatrix3D rotation;
      rotationQuat.ToMatrix(rotation);

      FLOAT3D prevRight = vleft;
      for (INDEX i = 0; i < numStops/2; ++i)
      {
        const FLOAT3D mid = prevRight * rotation;
        const FLOAT3D newRight = mid * rotation;
        Particle_RenderQuad3D_NEW(
          v0,
          v0 + prevRight,
          v0 + mid,
          v0 + newRight,
          FLOAT2D(uvX + uvIncrement*0.5f, 0.99f + uvYOffset),
          FLOAT2D(uvX, 0.01f + uvYOffset),
          FLOAT2D(uvX + uvIncrement*0.5f, 0.01f + uvYOffset),
          FLOAT2D(uvX + uvIncrement, 0.01f + uvYOffset),
          color);
        prevRight = newRight;
        uvX += uvIncrement;
      }
      if (numStops & 1) {
        const FLOAT uvLastIncrement = uvIncrement*0.5f;
        Particle_RenderQuad3D_NEW(
          v0, v0 + prevRight, v2, v0,
          FLOAT2D(uvX + uvLastIncrement*0.5f, 0.99f + uvYOffset),
          FLOAT2D(uvX, 0.01f + uvYOffset),
          FLOAT2D(uvX + uvLastIncrement, 0.01f + uvYOffset),
          FLOAT2D(uvX + uvLastIncrement*0.5f, 0.01f + uvYOffset),
          color);
        uvX += uvLastIncrement;
      }
    } else {
      const FLOAT uvWidth = angle * uvAspect;
      Particle_RenderQuad3D_NEW(
        v0, v1, v2, v0,
          FLOAT2D(uvX + uvWidth*0.5f, 0.99f + uvYOffset),
          FLOAT2D(uvX, 0.01f + uvYOffset),
          FLOAT2D(uvX + uvWidth, 0.01f + uvYOffset),
          FLOAT2D(uvX + uvWidth*0.5f, 0.01f + uvYOffset),
        color);
      uvX += uvWidth;
    }
  }

  //----------------------------------------------------------------------------------------------------
  void PrepareEntities()
  {
    outlineAverageCenter = FLOAT3D(0, 0, 0);
    INDEX numEntities = 0;
    INDEX i = 0;
    for (CEntityPointer* p_entityPointer = &m_glowEntity1; p_entityPointer != &m_glowEntityEnd; ++p_entityPointer, ++i)
    {
      EntityAndPlacement& ep = outlineEntities[i];
      ep = p_entityPointer->ep_pen;
      if (ep.entity)
      {
        ++numEntities;
        outlineAverageCenter += ep.pos;
      }
    }
    if (numEntities > 0) {
      outlineAverageCenter /= static_cast<FLOAT>(numEntities);
    }
  }

  //----------------------------------------------------------------------------------------------------
  void RenderParticles()
  {
    if (!m_bActive) {
      return;
    }
    CProjection3D* p_proj = Particle_GetProjection();
    if (!p_proj->IsPerspective()) {
      return;
    }
    const CPerspectiveProjection3D& proj = (const CPerspectiveProjection3D&)(*p_proj);

    extension3D = Get3DExtension();
    if (extension3D < MIN_EXTENSION) {
      return;
    }

    PrepareEntities();
    const FLOAT distanceMul = GetDistanceMul(proj);
    if (distanceMul <= 0.0f) {
      return;
    }
    const COLOR color = GetColor(distanceMul);

    CTextureObject& to = GetModelObject()->mo_toTexture;
    Particle_PrepareTexture(&to, (ParticleBlendType)m_blendType);

    uvAspect = static_cast<FLOAT>(to.GetHeight()) / to.GetWidth() * m_UVAspect;
    uvXOffset = GetUVXOffset();
    uvYOffset = GetUVYOffset();

    if (m_depthTesting) {
      shaEnableDepthTest();
    } else {
      shaDisableDepthTest();
    }

    if (m_type == OT_CONTOUR_AVG_DEPTH || m_type == OT_CONTOUR_FIXED_DEPTH)
    {
      CU_Clear();
      FLOAT averageZ = 0.0f;
      INDEX numEntities = 0;
      {for (INDEX i = 0; i < OUTLINE_COUNT; ++i)
      {
        const EntityAndPlacement& penEntity = outlineEntities[i];
        if (!penEntity.entity) {
          continue;
        }

        const FLOAT medianZ = AddTrianglesToCU(penEntity, proj);
        if (medianZ != 0.0f) {
          averageZ += medianZ;
          ++numEntities;
        }
      }} // for each entity

      averageZ /= static_cast<FLOAT>(numEntities);
      if (m_type == OT_CONTOUR_FIXED_DEPTH) {
        averageZ = -10.0f;
      }
      averageZ += m_averageDepthOffset;
      const FLOAT extension2D = Get2DExtension(averageZ, proj);
      const INDEX numContours = CU_UnitePolygons();
      for (INDEX i = 0; i < numContours; ++i)
      {
        FLOAT uvX = uvXOffset;
        FLOAT* points;
        const INDEX numPoints = CU_GetContour(i, &points);

        FLOAT x00 = points[(numPoints-2)*2]/CU_MULT;
        FLOAT y00 = points[(numPoints-2)*2+1]/CU_MULT;
        FLOAT x0 = points[(numPoints-1)*2]/CU_MULT;
        FLOAT y0 = points[(numPoints-1)*2+1]/CU_MULT;
        FLOAT3D v0 = ProjectCoordinateReverse(FLOAT3D(x0, y0, averageZ), proj);
        FLOAT2D prevNorm2D(y0 - y00, x00 - x0);
        prevNorm2D *= extension2D / prevNorm2D.Length();
        FLOAT x1 = points[0]/CU_MULT;
        FLOAT y1 = points[1]/CU_MULT;
        FLOAT2D norm2D(y1 - y0, x0 - x1);
        norm2D *= extension2D / norm2D.Length();
        for (INDEX j = 0; j < numPoints; ++j)
        {
          const FLOAT x2 = points[((j+1)%numPoints)*2]/CU_MULT;
          const FLOAT y2 = points[((j+1)%numPoints)*2+1]/CU_MULT;
          FLOAT2D nextNorm2D(y2 - y1, x1 - x2);
          nextNorm2D *= extension2D / nextNorm2D.Length();

          FLOAT2D v0n2D(x0 + norm2D(1), y0 + norm2D(2));
          FLOAT2D v1n2D(x1 + norm2D(1), y1 + norm2D(2));
          const FLOAT2D v0n2Dorig = v0n2D;
          const FLOAT uvWidth = FLOAT2D(x1 - x0, y1 - y0).Length() / extension2D * uvAspect;
          FLOAT v0nuvAdjustment = 0.0f;
          FLOAT v1nuvAdjustment = uvWidth;

          bool rightTurnPrev = false;
          if (RightTurn(prevNorm2D, norm2D)) {
            rightTurnPrev = true;
            IntersectEdges(v0n2D,
              FLOAT2D(x00 + prevNorm2D(1), y00 + prevNorm2D(2)), FLOAT2D(x0 + prevNorm2D(1), y0 + prevNorm2D(2)),
              v0n2D, v1n2D);
            v0nuvAdjustment = (v0n2Dorig - v0n2D).Length() / extension2D * uvAspect;
          }
          if (RightTurn(norm2D, nextNorm2D)) {
            IntersectEdges(v1n2D,
              v0n2D, v1n2D,
              FLOAT2D(x1 + nextNorm2D(1), y1 + nextNorm2D(2)), FLOAT2D(x2 + nextNorm2D(1), y2 + nextNorm2D(2)));
            v1nuvAdjustment = (v0n2Dorig - v1n2D).Length() / extension2D * uvAspect;
          }

          FLOAT3D v0n = ProjectCoordinateReverse(FLOAT3D(v0n2D(1), v0n2D(2), averageZ), proj);
          FLOAT3D v1n = ProjectCoordinateReverse(FLOAT3D(v1n2D(1), v1n2D(2), averageZ), proj);
          FLOAT3D v1 = ProjectCoordinateReverse(FLOAT3D(x1, y1, averageZ), proj);

          if (!rightTurnPrev) {
            const FLOAT2D v12D(x0 + prevNorm2D(1), y0 + prevNorm2D(2));
            RenderCorner(extension3D,
              v0, ProjectCoordinateReverse(FLOAT3D(v12D(1), v12D(2), averageZ), proj), v0n,
              uvX, color);
          }

          FLOAT uvAdj = 0.0f;
          if (!m_continuousUVx) {
            uvAdj = -uvWidth * 0.5f;
          }
          Particle_RenderQuad3D_NEW(
            v0, v0n, v1n, v1,
            FLOAT2D(uvX + uvAdj, 0.99f + uvYOffset),
            FLOAT2D(uvX + v0nuvAdjustment + uvAdj, 0.01f + uvYOffset),
            FLOAT2D(uvX + v1nuvAdjustment + uvAdj, 0.01f + uvYOffset),
            FLOAT2D(uvX + uvWidth + uvAdj, 0.99f + uvYOffset),
            color);

          if (m_continuousUVx) {
            uvX += uvWidth;
          } else {
            uvX = uvXOffset;
          }

          x00 = x0;
          y00 = y0;
          x0 = x1;
          y0 = y1;
          v0 = v1;
          prevNorm2D = norm2D;
          norm2D = nextNorm2D;
          x1 = x2;
          y1 = y2;
        }
      } // for each contour
    } // if (m_type == OT_CONTOUR_AVG_DEPTH || m_type == OT_CONTOUR_FIXED_DEPTH)
    else // if (m_type == OT_CONVEX_REAL_DEPTH || m_type == OT_CONVEX_AVG_DEPTH || m_type == OT_CONVEX_FIXED_DEPTH)
    {
      g_points2D.PopAll();
      FLOAT averageZ = 0.0f;
      INDEX numEntities = 0;
      {for (INDEX i = 0; i < OUTLINE_COUNT; ++i)
      {
        const EntityAndPlacement& penEntity = outlineEntities[i];
        if (!penEntity.entity) {
          continue;
        }

        const FLOAT medianZ = AddEdgesToGraham(penEntity, proj);
        if (medianZ != 0.0f) {
          averageZ += medianZ;
          ++numEntities;
        }
      }} // for each entity
      averageZ /= static_cast<FLOAT>(numEntities);
      if (m_type == OT_CONVEX_FIXED_DEPTH) {
        averageZ = -10.0f;
      }
      averageZ += m_averageDepthOffset;
      const FLOAT extension2D = Get2DExtension(averageZ, proj);

      // perform graham scan to find convex hull
      const INDEX pointsCount = g_points2D.Count();
      if (pointsCount >= 3)
      {
        const PIX2D pivot = (g_points2D[0].pos + g_points2D[1].pos + g_points2D[2].pos) / 3;
        g_pivot = &pivot;
        qsort(g_points2D.da_Pointers, pointsCount, sizeof(GrahamNode*), &CompareByPivotAngle);
        INDEX lowestIndex = 0;
        PIX2D lowest = g_points2D[0].pos;

        {for (INDEX i = 0; i < pointsCount; ++i) {
          const PIX2D& pos2D = g_points2D[i].pos;
          if (pos2D(2) < lowest(2) || (pos2D(2) == lowest(2) && pos2D(1) < lowest(1))) {
            lowest = pos2D;
            lowestIndex = i;
          }
          g_points2D[i].next = &g_points2D[(i + 1) % pointsCount];
          g_points2D[i].prev = &g_points2D[(i + pointsCount - 1) % pointsCount];
          g_points2D[i].pos = pos2D;
        }}
        GrahamNode* start = &g_points2D[lowestIndex];
        GrahamNode* v = start;
        INDEX numNodes = pointsCount;
        while (v->next != start)
        {
          if (LeftTurn(v->next->pos - v->pos, v->next->next->pos - v->pos))
          {
            v = v->next;
          } else {
            --numNodes;
            v->next = v->next->next;
            v->next->prev = v;
            if (v != start) {
              v = v->prev;
            }
          }
        }

        v = start;
        FLOAT uvX = uvXOffset;
        if (m_type == OT_CONVEX_AVG_DEPTH || m_type == OT_CONVEX_FIXED_DEPTH)
        {
          FLOAT3D vPrev(v->prev->pos(1), v->prev->pos(2), averageZ);
          FLOAT3D vPrev3D;
          vPrev3D = ProjectCoordinateReverse(vPrev, proj);
          FLOAT3D origv0;
          FLOAT3D origv1;
          FLOAT3D prevv2;
          bool origSaved = false;
          {for (INDEX i = 0; i < numNodes; ++i)
          {
            if (v->pos != v->prev->pos)
            {
              const FLOAT3D pos(v->pos(1), v->pos(2), averageZ);
              FLOAT3D norm(pos(2)-vPrev(2), vPrev(1)-pos(1), 0.0f);
              const FLOAT normLen = norm.Length();
              norm *= extension2D / normLen;
              const FLOAT uvWidth = normLen / extension2D * uvAspect;

              const FLOAT3D& v0 = vPrev3D;
              FLOAT3D v1 = ProjectCoordinateReverse(vPrev + norm, proj);
              FLOAT3D v2 = ProjectCoordinateReverse(pos + norm, proj);
              FLOAT3D v3 = ProjectCoordinateReverse(pos, proj);

              if (origSaved)
              {
                RenderCorner(extension3D, v0, prevv2, v1, uvX, color);
              } else {
                origSaved = true;
                origv0 = v0;
                origv1 = v1;
              }
              prevv2 = v2;

              FLOAT uvAdj = 0.0f;
              if (!m_continuousUVx) {
                uvAdj = -uvWidth * 0.5f;
              }
              Particle_RenderQuad3D_NEW(
                v0, v1, v2, v3,
                FLOAT2D(uvX + uvAdj, 0.99f + uvYOffset),
                FLOAT2D(uvX + uvAdj, 0.01f + uvYOffset),
                FLOAT2D(uvX + uvWidth + uvAdj, 0.01f + uvYOffset),
                FLOAT2D(uvX + uvWidth + uvAdj, 0.99f + uvYOffset),
                color);
              
              if (m_continuousUVx) {
                uvX += uvWidth;
              } else {
                uvX = uvXOffset;
              }
              
              vPrev = pos;
              vPrev3D = v3;
            }
            v = v->next;
          }}
          RenderCorner(extension3D, origv0, prevv2, origv1, uvX, color);
        } else { // if (m_type == OT_CONVEX_REAL_DEPTH)
          FLOAT3D origv0;
          FLOAT3D origv1;
          FLOAT3D prevv2;
          bool origSaved = false;
          {for (INDEX i = 0; i < numNodes; ++i)
          {
            if (v->pos != v->prev->pos)
            {
              const FLOAT dX = v->pos(1) - v->prev->pos(1);
              const FLOAT dY = v->pos(2) - v->prev->pos(2);
              const FLOAT len = sqrt(dX*dX + dY*dY);
              const FLOAT3D vpos3D = ProjectCoordinateReverse(FLOAT3D(v->pos(1), v->pos(2), -5), proj);
              const FLOAT3D vpos3Dnorm = ProjectCoordinateReverse(FLOAT3D(v->pos(1) + dY/len, v->pos(2) - dX/len, -5), proj);
              FLOAT3D norm3D = vpos3Dnorm - vpos3D;
              norm3D.Normalize();
              norm3D *= extension3D;

              const FLOAT3D& v0 = v->prev->posAbsolute;
              const FLOAT3D v1 = v0 + norm3D;
              const FLOAT3D& v3 = v->posAbsolute;
              const FLOAT3D v2 = v3 + norm3D;

              const FLOAT uvWidth = len / extension2D * uvAspect;
            
              if (origSaved)
              {
                RenderCorner(extension3D, v0, prevv2, v1, uvX, color);
              } else {
                origSaved = true;
                origv0 = v0;
                origv1 = v1;
              }
              prevv2 = v2;

              FLOAT uvAdj = 0.0f;
              if (!m_continuousUVx) {
                uvAdj = -uvWidth * 0.5f;
              }
              Particle_RenderQuad3D_NEW(
                v0, v1, v2, v3,
                FLOAT2D(uvX + uvAdj, 0.99f + uvYOffset),
                FLOAT2D(uvX + uvAdj, 0.01f + uvYOffset),
                FLOAT2D(uvX + uvWidth + uvAdj, 0.01f + uvYOffset),
                FLOAT2D(uvX + uvWidth + uvAdj, 0.99f + uvYOffset),
                color);
            
              if (m_continuousUVx) {
                uvX += uvWidth;
              } else {
                uvX = uvXOffset;
              }
            }
            v = v->next;
          }}
          RenderCorner(extension3D, origv0, prevv2, origv1, uvX, color);
        } // if (m_type == OT_CONVEX_REAL_DEPTH)
      }
    } // if (m_type == OT_CONVEX_REAL_DEPTH || m_type == OT_CONVEX_AVG_DEPTH || m_type == OT_CONVEX_FIXED_DEPTH)
    Particle_Flush();
    shaEnableDepthTest();
  }

procedures:
  Main()
  {
    InitAsEditorModel();
    SetPhysicsFlags(0);
    SetCollisionFlags(0);
    SetModel(MODEL_OUTLINE);
    try
    {
      if (m_texture.Length() > 0) {
        GetModelObject()->mo_toTexture.SetData_t(m_texture);
      } else {
        GetModelObject()->mo_toTexture.SetData_t(CTString("TexturesMP\\Detail\\White.tex"));
      }
      GetModelObject()->mo_toTexture.PlayAnim(m_textureAnimation, AOF_LOOPING);
    } catch (const char* error) {
      WarningMessage(error);
    }
    try
    {
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

    m_extension = ClampDn(m_extension, MIN_EXTENSION);
    m_fallOff = ClampDn(m_fallOff, 1.0f);
    m_hotSpot = Clamp(m_hotSpot, 0.0f, m_fallOff);

    EditGraphVariable(this, m_extensionScaleEdit, m_extensionScale);
    EditGraphVariable(this, m_alphaScaleEdit, m_alphaScale);
    EditGraphVariable(this, m_uvXOffsetEdit, m_uvXOffset);
    EditGraphVariable(this, m_uvYOffsetEdit, m_uvYOffset);
    RecacheGraphs();
    
    if (m_help) {
      m_help = FALSE;
      ShellExecute(NULL, NULL, "https://github.com/SeriousAlexej/ParametricParticles#outline", NULL, NULL, SW_SHOW);
    }

    autowait(0.1f);

    m_activatedTime = _pTimer->CurrentTick();
    wait()   
    {
      on (EActivate) :
      {
        m_activatedTime = _pTimer->CurrentTick();
        m_bActive = TRUE;
        resume;
      }
      on (EDeactivate) :
      {
        m_bActive = FALSE;
        resume;
      }
      otherwise() : { resume; }
    }
  }
};
