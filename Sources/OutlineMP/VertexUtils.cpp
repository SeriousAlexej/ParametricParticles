#include <Engine/Engine.h>
#define DECL_DLL __declspec(dllexport)
#include <ClipUnion/ClipUnion.h>
#include "VertexUtils.h"
#include "VertexUtils.hxx"

CDynamicStackArray<GrahamNode> g_points2D;

void EntityAndPlacement::operator=(CEntity* en)
{
  entity = en;
  if (!en)
    return;
  const CPlacement3D& lerpedPL = en->GetLerpedPlacement();
  MakeRotationMatrixFast(rot, lerpedPL.pl_OrientationAngle);
  pos = lerpedPL.pl_PositionVector;
}

namespace
{
  static FLOAT g_screenHeight;

  static void (*g_PointCallback)(const FLOAT3D& vClipped, const FLOAT3D& vWorld);
  static void (*g_TriangleCallback)();
  
//------------------------------------------------------------------------------------------------------------------------------------------
  inline FLOAT ProjFarClip(const CPerspectiveProjection3D& proj)
  {
    return proj.pr_FarClipDistance > 0 ? -proj.pr_FarClipDistance : -9999.0f;
  }
  
//------------------------------------------------------------------------------------------------------------------------------------------
  inline FLOAT ProjNearClip(const CPerspectiveProjection3D& proj)
  {
    return proj.pr_NearClipDistance > 0 ? -proj.pr_NearClipDistance : -0.25f;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  inline FLOAT GetMedianZ_AbsoluteAABBOX(const FLOATaabbox3D& bbox, const FLOAT3D& pos, const FLOATmatrix3D& rot, const CPerspectiveProjection3D& proj)
  {
    const FLOAT3D bboxSize = bbox.Size();
    const FLOAT3D bboxPoints[8] = {
      (bbox.minvect) * rot + pos,
      (bbox.minvect + FLOAT3D(bboxSize(1), 0, 0)) * rot + pos,
      (bbox.minvect + FLOAT3D(bboxSize(1), bboxSize(2), 0)) * rot + pos,
      (bbox.minvect + FLOAT3D(0, bboxSize(2), 0)) * rot + pos,
      (bbox.minvect + FLOAT3D(0, 0, bboxSize(3))) * rot + pos,
      (bbox.minvect + FLOAT3D(bboxSize(1), 0, bboxSize(3))) * rot + pos,
      (bbox.minvect + FLOAT3D(bboxSize(1), bboxSize(2), bboxSize(3))) * rot + pos,
      (bbox.minvect + FLOAT3D(0, bboxSize(2), bboxSize(3))) * rot + pos,
    };
    const FLOAT farClip = ProjFarClip(proj);
    const FLOAT nearClip = ProjNearClip(proj);
    FLOAT nearestZ = farClip;
    FLOAT farthestZ = nearClip;
    for (INDEX i = 0; i < 8; ++i)
    {
      FLOAT3D p;
      proj.PreClip(bboxPoints[i], p);
      const FLOAT z = Clamp(p(3), farClip, nearClip);
      nearestZ = Max(nearestZ, z);
      farthestZ = Min(farthestZ, z);
    }
    return nearestZ*0.5f + farthestZ*0.5f;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  inline FLOAT GetMedianZ_Brush(CBrush3D& brush, const FLOAT3D& pos, const FLOATmatrix3D& rot, const CPerspectiveProjection3D& proj)
  {
    const FLOATaabbox3D& bbox = brush.GetFirstMip()->bm_boxRelative;
    return GetMedianZ_AbsoluteAABBOX(bbox, pos, rot, proj);
  }
  
//------------------------------------------------------------------------------------------------------------------------------------------
  FLOAT GetMedianZ_Ska(CModelInstance& mi, const FLOAT3D& pos, const FLOATmatrix3D& rot, const CPerspectiveProjection3D& proj)
  {
    ColisionBox& box = (mi.mi_iCurentBBox >= 0 && mi.mi_iCurentBBox < mi.mi_cbAABox.Count()) ? mi.mi_cbAABox[mi.mi_iCurentBBox] : mi.mi_cbAllFramesBBox;
    FLOATaabbox3D bbox(box.Min(), box.Max());
    bbox.StretchByVector(mi.mi_vStretch);
    return GetMedianZ_AbsoluteAABBOX(bbox, pos, rot, proj);
  }
  
//------------------------------------------------------------------------------------------------------------------------------------------
  FLOAT GetMedianZ_Mdl(CModelObject& mo, const FLOAT3D& pos, const FLOATmatrix3D& rot, const CPerspectiveProjection3D& proj)
  {
    INDEX iFrame0, iFrame1;
    FLOAT fLerpRatio;
    mo.GetFrame(iFrame0, iFrame1, fLerpRatio);
    const CModelData& md = *((const CModelData*)mo.GetData());
    const FLOATaabbox3D& bbox1 = md.md_FrameInfos[iFrame0].mfi_Box;
    const FLOATaabbox3D& bbox2 = md.md_FrameInfos[iFrame1].mfi_Box;
    FLOATaabbox3D bbox(Lerp(bbox1.minvect, bbox2.minvect, fLerpRatio), Lerp(bbox1.maxvect, bbox2.maxvect, fLerpRatio));
    bbox.StretchByVector(mo.mo_Stretch);
    return GetMedianZ_AbsoluteAABBOX(bbox, pos, rot, proj);
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  FLOAT IterateTriangles_Ska(const EntityAndPlacement& ep, const FLOAT3D& viewerPos, const CPerspectiveProjection3D& proj)
  {
    CModelInstance& mi = *ep.entity->GetModelInstance();
    const FLOAT medianZ = GetMedianZ_Ska(mi, ep.pos, ep.rot, proj);
    if (medianZ >= ProjNearClip(proj) - 0.001f)
      return 0.0f;

    ska_fLODMul = _pShell->GetFLOAT("ska_fLODMul");
    ska_fLODAdd = _pShell->GetFLOAT("ska_fLODAdd");

    MatrixVectorToMatrix12(_mObjectToAbs, ep.rot, ep.pos);

    FLOAT3D objectViewPosition;
    proj.PreClip(ep.pos, objectViewPosition);
    _fDistanceFactor = proj.MipFactor(Min(objectViewPosition(3), 0.0f));

    INDEX iOldParentBoneID = mi.mi_iParentBoneID;

    mi.mi_iParentBoneID = -1;
  
    MakeIdentityMatrix(_mAbsToViewer);
    CalculateRenderingData(mi);

    INDEX ctrmsh = _aRenModels.Count();
    for (int irmsh = 1; irmsh < ctrmsh; ++irmsh)
    {
      RenModel& rm = _aRenModels[irmsh];
      INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
      for (int imsh = rm.rm_iFirstMesh; imsh < ctmsh; ++imsh)
      {
        RenMesh& rmsh = _aRenMesh[imsh];
        PrepareMeshForRendering(rmsh, rm.rm_iSkeletonLODIndex);

        const MeshLOD& mlod = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex];
        const INDEX numSurfaces = mlod.mlod_aSurfaces.Count();
        for (INDEX surf = 0; surf < numSurfaces; ++surf)
        {
          const MeshSurface& surface = mlod.mlod_aSurfaces[surf];

          const INDEX numTriangles = surface.msrf_aTriangles.Count();
          for (INDEX tri = 0; tri < numTriangles; ++tri)
          {
            const MeshTriangle& triangle = surface.msrf_aTriangles[tri];
            const MeshVertex& vert0 = _pavFinalVertices[surface.msrf_iFirstVertex + triangle.iVertex[0]];
            const MeshVertex& vert1 = _pavFinalVertices[surface.msrf_iFirstVertex + triangle.iVertex[1]];
            const MeshVertex& vert2 = _pavFinalVertices[surface.msrf_iFirstVertex + triangle.iVertex[2]];
            const FLOAT3D v0(vert0.x, vert0.y, vert0.z);
            const FLOAT3D v1(vert1.x, vert1.y, vert1.z);
            const FLOAT3D v2(vert2.x, vert2.y, vert2.z);
            
            const FLOAT3D polyNormal = (v2-v1)*(v0-v1);
            const bool backface = (v0 - viewerPos) % polyNormal >= 0;
            if (backface)
              continue;

            const FLOAT3D edges[3][2] = { {v0, v1}, {v1, v2}, {v2, v0} };
            for (INDEX j = 0; j < 3; ++j)
            {
              const FLOAT3D& e0 = edges[j][0];
              const FLOAT3D& e1 = edges[j][1];
              FLOAT3D vClipped0, vClipped1;
              proj.PreClip(e0, vClipped0);
              proj.PreClip(e1, vClipped1);
              const ULONG clipFlags = proj.ClipLine(vClipped0, vClipped1);
              if (clipFlags != LCF_EDGEREMOVED)
              {
                proj.PostClip(vClipped0, vClipped0);
                proj.PostClip(vClipped1, vClipped1);

                g_PointCallback(vClipped0, e0);
                if ((clipFlags >> LCS_VERTEX1) != LCF_UNCLIPPED)
                  g_PointCallback(vClipped1, e1);
              }
            }
            g_TriangleCallback();
          } // for each triangle
        } // for each surface
      } // for each mesh
    } // for each model
    mi.mi_iParentBoneID = iOldParentBoneID;
 
    _aRenModels.PopAll();
    _aRenBones.PopAll();
    _aRenMesh.PopAll();
    _aRenWeights.PopAll();
    _aRenMorph.PopAll();
    return medianZ;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void GetModelVertices_CU(
    CModelObject& mo,
    const FLOATmatrix3D& mRotation,
    const FLOAT3D& vPosition,
    FLOAT fMipFactor,
    const FLOAT3D& viewerPos,
    const CPerspectiveProjection3D& proj)
  {
    INDEX iFrame0, iFrame1;
    FLOAT fLerpRatio;
    CModelData* pmd = mo.GetData();

    struct ModelFrameVertex16 *pFrame16_0, *pFrame16_1;
    struct ModelFrameVertex8  *pFrame8_0,  *pFrame8_1;

    mo.GetFrame(iFrame0, iFrame1, fLerpRatio);

    if (pmd->md_Flags & MF_COMPRESSED_16BIT)
    {
      pFrame16_0 = &pmd->md_FrameVertices16[iFrame0 * pmd->md_VerticesCt];
      pFrame16_1 = &pmd->md_FrameVertices16[iFrame1 * pmd->md_VerticesCt];
    } else {
      pFrame8_0 = &pmd->md_FrameVertices8[iFrame0 * pmd->md_VerticesCt];
      pFrame8_1 = &pmd->md_FrameVertices8[iFrame1 * pmd->md_VerticesCt];
    }

    FLOAT3D& vDataStretch = pmd->md_Stretch;
    FLOAT3D& vObjectStretch = mo.mo_Stretch;
    FLOAT3D vStretch, vOffset;
    vStretch(1) = vDataStretch(1)*vObjectStretch(1);
    vStretch(2) = vDataStretch(2)*vObjectStretch(2);
    vStretch(3) = vDataStretch(3)*vObjectStretch(3);
    _vStretch = vStretch;
    _vOffset = vOffset = pmd->md_vCompressedCenter;
  
    BOOL bXInverted = vStretch(1)<0;
    BOOL bYInverted = vStretch(2)<0;
    BOOL bZInverted = vStretch(3)<0;
    BOOL bInverted  = bXInverted!=bYInverted!=bZInverted;

    if (mo.mo_Stretch != FLOAT3D( 1.0f, 1.0f, 1.0f))
      fMipFactor -= Log2(Max(mo.mo_Stretch(1),Max(mo.mo_Stretch(2),mo.mo_Stretch(3))));

    fMipFactor = fMipFactor*mdl_fLODMul+mdl_fLODAdd;

    INDEX iMipLevel = mo.GetMipModel(fMipFactor);
    ModelMipInfo& mmi = pmd->md_MipInfos[iMipLevel];
    INDEX iStartElem = 0;

    if (pmd->md_Flags & MF_COMPRESSED_16BIT)
    {
      {FOREACHINSTATICARRAY(mmi.mmpi_MappingSurfaces, MappingSurface, itms)
      {
        const MappingSurface& ms = *itms;
        const ULONG ulFlags = ms.ms_ulRenderingFlags;
        if ((ulFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx == 0) // empty and invisible surfaces are last in the list
          break;

        const INDEX numTriangles = ms.ms_ctSrfEl / 3;
        for (INDEX i = 0; i < numTriangles; ++i)
        {
          const INDEX v0i = mmi.mmpi_auwMipToMdl[mmi.mmpi_auwSrfToMip[mmi.mmpi_aiElements[iStartElem + i*3 + 0]]];
          const INDEX v1i = mmi.mmpi_auwMipToMdl[mmi.mmpi_auwSrfToMip[mmi.mmpi_aiElements[iStartElem + i*3 + 1]]];
          const INDEX v2i = mmi.mmpi_auwMipToMdl[mmi.mmpi_auwSrfToMip[mmi.mmpi_aiElements[iStartElem + i*3 + 2]]];
          ModelFrameVertex16& mfv00 = pFrame16_0[v0i];
          ModelFrameVertex16& mfv10 = pFrame16_1[v0i];
          FLOAT3D v0(
            (Lerp((FLOAT)mfv00.mfv_SWPoint(1), (FLOAT)mfv10.mfv_SWPoint(1), fLerpRatio)-vOffset(1))*vStretch(1),
            (Lerp((FLOAT)mfv00.mfv_SWPoint(2), (FLOAT)mfv10.mfv_SWPoint(2), fLerpRatio)-vOffset(2))*vStretch(2),
            (Lerp((FLOAT)mfv00.mfv_SWPoint(3), (FLOAT)mfv10.mfv_SWPoint(3), fLerpRatio)-vOffset(3))*vStretch(3));
        
          ModelFrameVertex16& mfv01 = pFrame16_0[v1i];
          ModelFrameVertex16& mfv11 = pFrame16_1[v1i];
          FLOAT3D v1(
            (Lerp((FLOAT)mfv01.mfv_SWPoint(1), (FLOAT)mfv11.mfv_SWPoint(1), fLerpRatio)-vOffset(1))*vStretch(1),
            (Lerp((FLOAT)mfv01.mfv_SWPoint(2), (FLOAT)mfv11.mfv_SWPoint(2), fLerpRatio)-vOffset(2))*vStretch(2),
            (Lerp((FLOAT)mfv01.mfv_SWPoint(3), (FLOAT)mfv11.mfv_SWPoint(3), fLerpRatio)-vOffset(3))*vStretch(3));
        
          ModelFrameVertex16& mfv02 = pFrame16_0[v2i];
          ModelFrameVertex16& mfv12 = pFrame16_1[v2i];
          FLOAT3D v2(
            (Lerp((FLOAT)mfv02.mfv_SWPoint(1), (FLOAT)mfv12.mfv_SWPoint(1), fLerpRatio)-vOffset(1))*vStretch(1),
            (Lerp((FLOAT)mfv02.mfv_SWPoint(2), (FLOAT)mfv12.mfv_SWPoint(2), fLerpRatio)-vOffset(2))*vStretch(2),
            (Lerp((FLOAT)mfv02.mfv_SWPoint(3), (FLOAT)mfv12.mfv_SWPoint(3), fLerpRatio)-vOffset(3))*vStretch(3));
          v0 = v0*mRotation+vPosition;
          v1 = v1*mRotation+vPosition;
          v2 = v2*mRotation+vPosition;
          const FLOAT3D polyNormal = (v2-v1)*(v0-v1);
          const bool backface = (v0 - viewerPos) % polyNormal >= 0;
          if (backface && !(ulFlags&SRF_DOUBLESIDED))
            continue;

          const FLOAT3D edges[3][2] = { {v0, v1}, {v1, v2}, {v2, v0} };
          for (INDEX j = 0; j < 3; ++j)
          {
            const FLOAT3D& e0 = backface ? edges[2-j][1] : edges[j][0];
            const FLOAT3D& e1 = backface ? edges[2-j][0] : edges[j][1];
            FLOAT3D vClipped0, vClipped1;
            proj.PreClip(e0, vClipped0);
            proj.PreClip(e1, vClipped1);
            const ULONG clipFlags = proj.ClipLine(vClipped0, vClipped1);
            if (clipFlags != LCF_EDGEREMOVED)
            {
              proj.PostClip(vClipped0, vClipped0);
              proj.PostClip(vClipped1, vClipped1);

              g_PointCallback(vClipped0, e0);
              if ((clipFlags >> LCS_VERTEX1) != LCF_UNCLIPPED)
                g_PointCallback(vClipped1, e1);
            }
          }
          g_TriangleCallback();
        }
        iStartElem += ms.ms_ctSrfEl;
      }}
     } else { // --------------------------- 8 BIT -------------------------------
      {FOREACHINSTATICARRAY(mmi.mmpi_MappingSurfaces, MappingSurface, itms)
      {
        const MappingSurface& ms = *itms;
        const ULONG ulFlags = ms.ms_ulRenderingFlags;
        if ((ulFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx == 0) // empty and invisible surfaces are last in the list
          break;

        const INDEX numTriangles = ms.ms_ctSrfEl / 3;
        for (INDEX i = 0; i < numTriangles; ++i)
        {
          const INDEX v0i = mmi.mmpi_auwMipToMdl[mmi.mmpi_auwSrfToMip[mmi.mmpi_aiElements[iStartElem + i*3 + 0]]];
          const INDEX v1i = mmi.mmpi_auwMipToMdl[mmi.mmpi_auwSrfToMip[mmi.mmpi_aiElements[iStartElem + i*3 + 1]]];
          const INDEX v2i = mmi.mmpi_auwMipToMdl[mmi.mmpi_auwSrfToMip[mmi.mmpi_aiElements[iStartElem + i*3 + 2]]];
          ModelFrameVertex8& mfv00 = pFrame8_0[v0i];
          ModelFrameVertex8& mfv10 = pFrame8_1[v0i];
          FLOAT3D v0(
            (Lerp((FLOAT)mfv00.mfv_SBPoint(1), (FLOAT)mfv10.mfv_SBPoint(1), fLerpRatio)-vOffset(1))*vStretch(1),
            (Lerp((FLOAT)mfv00.mfv_SBPoint(2), (FLOAT)mfv10.mfv_SBPoint(2), fLerpRatio)-vOffset(2))*vStretch(2),
            (Lerp((FLOAT)mfv00.mfv_SBPoint(3), (FLOAT)mfv10.mfv_SBPoint(3), fLerpRatio)-vOffset(3))*vStretch(3));
        
          ModelFrameVertex8& mfv01 = pFrame8_0[v1i];
          ModelFrameVertex8& mfv11 = pFrame8_1[v1i];
          FLOAT3D v1(
            (Lerp((FLOAT)mfv01.mfv_SBPoint(1), (FLOAT)mfv11.mfv_SBPoint(1), fLerpRatio)-vOffset(1))*vStretch(1),
            (Lerp((FLOAT)mfv01.mfv_SBPoint(2), (FLOAT)mfv11.mfv_SBPoint(2), fLerpRatio)-vOffset(2))*vStretch(2),
            (Lerp((FLOAT)mfv01.mfv_SBPoint(3), (FLOAT)mfv11.mfv_SBPoint(3), fLerpRatio)-vOffset(3))*vStretch(3));
        
          ModelFrameVertex8& mfv02 = pFrame8_0[v2i];
          ModelFrameVertex8& mfv12 = pFrame8_1[v2i];
          FLOAT3D v2(
            (Lerp((FLOAT)mfv02.mfv_SBPoint(1), (FLOAT)mfv12.mfv_SBPoint(1), fLerpRatio)-vOffset(1))*vStretch(1),
            (Lerp((FLOAT)mfv02.mfv_SBPoint(2), (FLOAT)mfv12.mfv_SBPoint(2), fLerpRatio)-vOffset(2))*vStretch(2),
            (Lerp((FLOAT)mfv02.mfv_SBPoint(3), (FLOAT)mfv12.mfv_SBPoint(3), fLerpRatio)-vOffset(3))*vStretch(3));
          v0 = v0*mRotation+vPosition;
          v1 = v1*mRotation+vPosition;
          v2 = v2*mRotation+vPosition;
          const FLOAT3D polyNormal = (v2-v1)*(v0-v1);
          const bool backface = (v0 - viewerPos) % polyNormal >= 0;
          if (backface && !(ulFlags&SRF_DOUBLESIDED))
            continue;

          const FLOAT3D edges[3][2] = { {v0, v1}, {v1, v2}, {v2, v0} };
          for (INDEX j = 0; j < 3; ++j)
          {
            const FLOAT3D& e0 = backface ? edges[2-j][1] : edges[j][0];
            const FLOAT3D& e1 = backface ? edges[2-j][0] : edges[j][1];
            FLOAT3D vClipped0, vClipped1;
            proj.PreClip(e0, vClipped0);
            proj.PreClip(e1, vClipped1);
            const ULONG clipFlags = proj.ClipLine(vClipped0, vClipped1);
            if (clipFlags != LCF_EDGEREMOVED)
            {
              proj.PostClip(vClipped0, vClipped0);
              proj.PostClip(vClipped1, vClipped1);

              g_PointCallback(vClipped0, e0);
              if ((clipFlags >> LCS_VERTEX1) != LCF_UNCLIPPED)
                g_PointCallback(vClipped1, e1);
            }
          }
          g_TriangleCallback();
        }
        iStartElem += ms.ms_ctSrfEl;
      }}
     }

    FOREACHINLIST(CAttachmentModelObject, amo_lnInMain, mo.mo_lhAttachments, itamo) {
      CAttachmentModelObject *pamo = itamo;
      CModelData* pmd=pamo->amo_moModelObject.GetData();
      if(pmd==NULL || pmd->md_Flags&(MF_FACE_FORWARD|MF_HALF_FACE_FORWARD)) continue;
      FLOATmatrix3D mNew = mRotation;
      FLOAT3D vNew = vPosition;
      GetAttachmentMatrices_CU(mo, pamo, mNew, vNew);
      GetModelVertices_CU(pamo->amo_moModelObject, mNew, vNew, fMipFactor, viewerPos, proj);
    }
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  FLOAT IterateTriangles_Mdl(const EntityAndPlacement& ep, const FLOAT3D& viewerPos, const CPerspectiveProjection3D& proj)
  {
    const FLOAT medianZ = GetMedianZ_Mdl(*ep.entity->GetModelObject(), ep.pos, ep.rot, proj);
    if (medianZ >= ProjNearClip(proj) - 0.001f)
      return 0.0f;

    mdl_fLODMul = _pShell->GetFLOAT("mdl_fLODMul");
    mdl_fLODAdd = _pShell->GetFLOAT("mdl_fLODAdd");

    FLOAT3D objectViewPosition;
    proj.PreClip(ep.pos, objectViewPosition);
    const FLOAT mipFactor = proj.MipFactor(Min(objectViewPosition(3), 0.0f));

    GetModelVertices_CU(
      *ep.entity->GetModelObject(),
      ep.rot,
      ep.pos,
      mipFactor,
      viewerPos,
      proj);

    return medianZ;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  FLOAT IterateTriangles_Brush(const EntityAndPlacement& ep, const FLOAT3D& viewerPos, const CPerspectiveProjection3D& proj)
  {
    CBrush3D* p_brush3D = ep.entity->GetBrush();
    if (!p_brush3D)
      return 0.0f;

    FLOAT3D objectViewPosition;
    proj.PreClip(ep.pos, objectViewPosition);
    const FLOAT mipFactor = _wrpWorldRenderPrefs.GetCurrentMipBrushingFactor(-objectViewPosition(3)*TanFast(proj.ppr_FOVWidth*0.5f));

    CBrushMip* p_mip = p_brush3D->GetBrushMipByDistance(mipFactor);
    if (!p_mip)
      return 0.0f;

    const FLOAT medianZ = GetMedianZ_Brush(*p_brush3D, ep.pos, ep.rot, proj);
    if (medianZ >= ProjNearClip(proj) - 0.001f)
      return 0.0f;

    FOREACHINDYNAMICARRAY(p_mip->bm_abscSectors, CBrushSector, itbsc)
    {
      FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itpoly)
      {
        if (itpoly->bpo_ulFlags & BPOF_INVISIBLE || ((itpoly->bpo_ulFlags & BPOF_PORTAL) && !(itpoly->bpo_ulFlags & BPOF_TRANSLUCENT))) {
          continue;
        }
        const FLOAT3D anyVertex = (itpoly->bpo_apbvxTriangleVertices[0]->bvx_vRelative) * ep.rot + ep.pos;
        const FLOAT3D polyNormal = itpoly->bpo_pbplPlane->bpl_plRelative * ep.rot;
        const bool backface = (anyVertex - viewerPos) % polyNormal >= 0;
        if (backface && !(itpoly->bpo_ulFlags & BPOF_DOUBLESIDED))
          continue;

        const INDEX numTriangles = itpoly->bpo_aiTriangleElements.Count() / 3;
        for (INDEX i = 0; i < numTriangles; ++i)
        {
          const INDEX iVtx0 = itpoly->bpo_aiTriangleElements[i*3];
          const INDEX iVtx1 = itpoly->bpo_aiTriangleElements[i*3 + 1];
          const INDEX iVtx2 = itpoly->bpo_aiTriangleElements[i*3 + 2];
          const FLOAT3D v0 = itpoly->bpo_apbvxTriangleVertices[iVtx0]->bvx_vRelative * ep.rot + ep.pos;
          const FLOAT3D v1 = itpoly->bpo_apbvxTriangleVertices[iVtx1]->bvx_vRelative * ep.rot + ep.pos;
          const FLOAT3D v2 = itpoly->bpo_apbvxTriangleVertices[iVtx2]->bvx_vRelative * ep.rot + ep.pos;
    
          const FLOAT3D edges[3][2] = { {v0, v1}, {v1, v2}, {v2, v0} };
          for (INDEX j = 0; j < 3; ++j)
          {
            const FLOAT3D& v0 = backface ? edges[2-j][1] : edges[j][0];
            const FLOAT3D& v1 = backface ? edges[2-j][0] : edges[j][1];
            FLOAT3D vClipped0, vClipped1;
            proj.PreClip(v0, vClipped0);
            proj.PreClip(v1, vClipped1);
            const ULONG clipFlags = proj.ClipLine(vClipped0, vClipped1);
            if (clipFlags != LCF_EDGEREMOVED)
            {
              proj.PostClip(vClipped0, vClipped0);
              proj.PostClip(vClipped1, vClipped1);

              g_PointCallback(vClipped0, v0);
              if ((clipFlags >> LCS_VERTEX1) != LCF_UNCLIPPED)
                g_PointCallback(vClipped1, v1);
            }
          }
          g_TriangleCallback();
        } // for each triangle
      } // for each polygon
    } // for each sector
    return medianZ;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  FLOAT IterateEdges_Brush(const EntityAndPlacement& ep, const FLOAT3D& viewerPos, const CPerspectiveProjection3D& proj)
  {
    CBrush3D* p_brush3D = ep.entity->GetBrush();
    if (!p_brush3D)
      return 0.0f;

    FLOAT3D objectViewPosition;
    proj.PreClip(ep.pos, objectViewPosition);
    const FLOAT mipFactor = _wrpWorldRenderPrefs.GetCurrentMipBrushingFactor(-objectViewPosition(3)*TanFast(proj.ppr_FOVWidth*0.5f));

    CBrushMip* p_mip = p_brush3D->GetBrushMipByDistance(mipFactor);
    if (!p_mip)
      return 0.0f;
    
    const FLOAT medianZ = GetMedianZ_Brush(*p_brush3D, ep.pos, ep.rot, proj);
    if (medianZ >= ProjNearClip(proj) - 0.001f)
      return 0.0f;

    FOREACHINDYNAMICARRAY(p_mip->bm_abscSectors, CBrushSector, itbsc)
    {
      FOREACHINSTATICARRAY(itbsc->bsc_abedEdges, CBrushEdge, itedge)
      {
        const FLOAT3D& v0orig = itedge->bed_pbvxVertex0->bvx_vRelative;
        const FLOAT3D& v1orig = itedge->bed_pbvxVertex1->bvx_vRelative;
        const FLOAT3D v0 = v0orig * ep.rot + ep.pos;
        const FLOAT3D v1 = v1orig * ep.rot + ep.pos;
        FLOAT3D vClipped0, vClipped1;
        proj.PreClip(v0, vClipped0);
        proj.PreClip(v1, vClipped1);
        if (proj.ClipLine(vClipped0, vClipped1) != LCF_EDGEREMOVED)
        {
          proj.PostClip(vClipped0, vClipped0);
          proj.PostClip(vClipped1, vClipped1);
          g_PointCallback(vClipped0, v0);
          g_PointCallback(vClipped1, v1);
        }
      } // for each edge
    } // for each sector
    return medianZ;
  }
  
//------------------------------------------------------------------------------------------------------------------------------------------
  static FLOAT g_polygonVertices[12];
  static INDEX g_polygonVerticesCount = 0;
  void PointCallback_CU(const FLOAT3D& vClipped, const FLOAT3D& vWorld)
  {
    (void)vWorld;
    g_polygonVertices[g_polygonVerticesCount++] = vClipped(1) * CU_MULT;
    g_polygonVertices[g_polygonVerticesCount++] = (g_screenHeight - vClipped(2)) * CU_MULT;
  }

  void TriangleCallback_CU()
  {
    CU_AddPolygon(g_polygonVertices, g_polygonVerticesCount / 2);
    g_polygonVerticesCount = 0;
  }

  void PointCallback_Graham(const FLOAT3D& vClipped, const FLOAT3D& vWorld)
  {
    GrahamNode& p = g_points2D.Push();
    p.posAbsolute = vWorld;
    p.pos(1) = vClipped(1);
    p.pos(2) = g_screenHeight - vClipped(2);
  }

  void TriangleCallback_Dummy()
  {
  }
} // anonymous namespace

//------------------------------------------------------------------------------------------------------------------------------------------
FLOAT AddTrianglesToCU(const EntityAndPlacement& ep, const CPerspectiveProjection3D& proj)
{
  g_screenHeight = proj.pr_ScreenBBox.Size()(2);
  g_PointCallback = &PointCallback_CU;
  g_TriangleCallback = &TriangleCallback_CU;
  const FLOAT3D viewerPos = ProjectCoordinateReverse(FLOAT3D(0, 0, 0), proj);
  const CEntity& entity = *ep.entity;

  if (entity.en_RenderType == CEntity::RT_FIELDBRUSH || entity.en_RenderType == CEntity::RT_BRUSH)
    return IterateTriangles_Brush(ep, viewerPos, proj);
  else if (entity.en_RenderType == CEntity::RT_MODEL || entity.en_RenderType == CEntity::RT_EDITORMODEL)
    return IterateTriangles_Mdl(ep, viewerPos, proj);
  else if (entity.en_RenderType == CEntity::RT_SKAMODEL || entity.en_RenderType == CEntity::RT_SKAEDITORMODEL)
    return IterateTriangles_Ska(ep, viewerPos, proj);

  return 0.0f;
}

//------------------------------------------------------------------------------------------------------------------------------------------
FLOAT AddEdgesToGraham(const EntityAndPlacement& ep, const CPerspectiveProjection3D& proj)
{
  g_screenHeight = proj.pr_ScreenBBox.Size()(2);
  g_PointCallback = &PointCallback_Graham;
  g_TriangleCallback = &TriangleCallback_Dummy;
  const FLOAT3D viewerPos = ProjectCoordinateReverse(FLOAT3D(0, 0, 0), proj);
  const CEntity& entity = *ep.entity;

  if (entity.en_RenderType == CEntity::RT_FIELDBRUSH || entity.en_RenderType == CEntity::RT_BRUSH)
    return IterateEdges_Brush(ep, viewerPos, proj);
  if (entity.en_RenderType == CEntity::RT_MODEL || entity.en_RenderType == CEntity::RT_EDITORMODEL)
    return IterateTriangles_Mdl(ep, viewerPos, proj);
  if (entity.en_RenderType == CEntity::RT_SKAMODEL || entity.en_RenderType == CEntity::RT_SKAEDITORMODEL)
    return IterateTriangles_Ska(ep, viewerPos, proj);

  return 0.0f;
}

//------------------------------------------------------------------------------------------------------------------------------------------
FLOAT3D ProjectCoordinateReverse(const FLOAT3D& v3dViewPoint, const CPerspectiveProjection3D& proj)
{
  FLOAT3D v3dObjectPoint = v3dViewPoint;
  v3dObjectPoint(1) = (v3dObjectPoint(1) - proj.pr_ScreenCenter(1)) / proj.ppr_PerspectiveRatios(1) * v3dObjectPoint(3);
  v3dObjectPoint(2) = (v3dObjectPoint(2) - proj.pr_ScreenCenter(2)) / proj.ppr_PerspectiveRatios(2) * v3dObjectPoint(3);
  return (v3dObjectPoint - proj.pr_TranslationVector) * (!proj.pr_RotationMatrix);
}
