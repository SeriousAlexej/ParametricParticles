/*
This file mostly contains some copy-pasted code from 1.10 but with small tweaks to only extract geometry edges
*/
namespace
{
  // MDL
  static BOOL _b16Bit;
  static FLOAT _fRatio;
  static FLOAT mdl_fLODMul;
  static FLOAT mdl_fLODAdd;
  static FLOAT3D _vStretch;
  static FLOAT3D _vOffset;
  static ModelFrameVertex8*  _pFrame8_0;
  static ModelFrameVertex8*  _pFrame8_1;
  static ModelFrameVertex16* _pFrame16_0;
  static ModelFrameVertex16* _pFrame16_1;
  // SKA
  static Matrix12 _mObjectToAbs;
  static Matrix12 _mAbsToViewer;
  static Matrix12 _mObjToView;
  static Matrix12 _mObjToViewStretch;
  static FLOAT _fDistanceFactor;
  static FLOAT ska_fLODMul;
  static FLOAT ska_fLODAdd;
  static CStaticStackArray<RenModel> _aRenModels;
  static CStaticStackArray<RenBone> _aRenBones;
  static CStaticStackArray<RenMesh> _aRenMesh;
  static CStaticStackArray<RenMorph> _aRenMorph;
  static CStaticStackArray<RenWeight> _aRenWeights;
  static CStaticStackArray<MeshVertex> _aMorphedVtxs;
  static CStaticStackArray<MeshVertex> _aFinalVtxs;
  static MeshVertex* _pavFinalVertices = NULL;
  static INDEX _ctFinalVertices;

//------------------------------------------------------------------------------------------------------------------------------------------
  inline static void TransformVector(FLOAT3 &v, const Matrix12 &m)
  {
    float x = v[0];
    float y = v[1];
    float z = v[2];
    v[0] = m[0]*x + m[1]*y + m[ 2]*z + m[ 3];
    v[1] = m[4]*x + m[5]*y + m[ 6]*z + m[ 7];
    v[2] = m[8]*x + m[9]*y + m[10]*z + m[11];
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  inline static void RemoveRotationFromMatrix(Matrix12 &mat)
  {
    mat[ 0] = 1; mat[ 1] = 0; mat[ 2] = 0; 
    mat[ 4] = 0; mat[ 5] = 1; mat[ 6] = 0; 
    mat[ 8] = 0; mat[ 9] = 0; mat[10] = 1;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  inline static void MakeIdentityMatrix(Matrix12 &mat)
  {
    memset(&mat,0,sizeof(mat));
    mat[0]  = 1;
    mat[5]  = 1;
    mat[10] = 1;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  inline static void MatrixCopy(Matrix12 &c, const Matrix12 &m)
  {
    memcpy(&c,&m,sizeof(c));
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  inline static void MakeStretchMatrix(Matrix12& c, const FLOAT3D& v)
  {
    c[ 0] = v(1); c[ 1] = 0.0f; c[ 2] = 0.0f; c[ 3] = 0.0f; 
    c[ 4] = 0.0f; c[ 5] = v(2); c[ 6] = 0.0f; c[ 7] = 0.0f; 
    c[ 8] = 0.0f; c[ 9] = 0.0f; c[10] = v(3); c[11] = 0.0f; 
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  INDEX GetMeshLOD(CMesh& msh)
  {
    FLOAT fMinDistance = 1000000.0f;
    INDEX iMeshLod = -1;

    INDEX ctmlods = msh.msh_aMeshLODs.Count();
    for(INDEX imlod=0;imlod<ctmlods;imlod++) {
      MeshLOD &mlod = msh.msh_aMeshLODs[imlod];
      FLOAT fLodMaxDistance = mlod.mlod_fMaxDistance*ska_fLODMul+ska_fLODAdd;

      if(_fDistanceFactor<fLodMaxDistance && fLodMaxDistance<fMinDistance) {
        fMinDistance = fLodMaxDistance;
        iMeshLod = imlod;
      }
    }
    return iMeshLod;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  INDEX GetSkeletonLOD(CSkeleton& sk)
  {
    FLOAT fMinDistance = 1000000.0f;
    INDEX iSkeletonLod = -1;

    INDEX ctslods = sk.skl_aSkeletonLODs.Count();
    for(INDEX islod=0;islod<ctslods;islod++) {
      SkeletonLOD &slod = sk.skl_aSkeletonLODs[islod];
      FLOAT fLodMaxDistance = slod.slod_fMaxDistance*ska_fLODMul+ska_fLODAdd;

      if(_fDistanceFactor < fLodMaxDistance && fLodMaxDistance < fMinDistance) {
        fMinDistance = fLodMaxDistance;
        iSkeletonLod = islod;
      }
    }
    return iSkeletonLod;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void MakeRootModel()
  {
    RenModel &rm = _aRenModels.Push();
    rm.rm_pmiModel = NULL;
    rm.rm_iFirstBone = 0;
    rm.rm_ctBones = 1;
    rm.rm_iParentBoneIndex = -1;
    rm.rm_iParentModelIndex = -1;
  
    RenBone &rb = _aRenBones.Push();
    rb.rb_iParentIndex = -1;
    rb.rb_psbBone = NULL;
    memset(&rb.rb_apPos,0,sizeof(AnimPos));
    memset(&rb.rb_arRot,0,sizeof(AnimRot));
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void SetObjectMatrices(const CModelInstance &mi)
  {
    MatrixMultiply(_mObjToView, _mAbsToViewer, _mObjectToAbs);
    Matrix12 mStretch;
    MakeStretchMatrix(mStretch, mi.mi_vStretch);
    MatrixMultiply(_mObjToViewStretch, _mObjToView, mStretch);
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  BOOL FindBone(int iBoneID, INDEX* piBoneIndex, CModelInstance* pmi, INDEX iSkeletonLod)
  {
    if(pmi->mi_psklSkeleton == NULL) return FALSE;
    if(iSkeletonLod < 0) return FALSE;

    INDEX ctslods = pmi->mi_psklSkeleton->skl_aSkeletonLODs.Count();
    if(ctslods<1) return FALSE;
    if(iSkeletonLod >= ctslods)
      iSkeletonLod = 0;

    SkeletonLOD &slod = pmi->mi_psklSkeleton->skl_aSkeletonLODs[iSkeletonLod];
    for(int i=0;i<slod.slod_aBones.Count();i++) {
      if(iBoneID == slod.slod_aBones[i].sb_iID)
        return TRUE;
      *piBoneIndex += 1;
    }

    INDEX ctmich = pmi->mi_cmiChildren.Count();
    for(INDEX imich =0;imich<ctmich;imich++)
      if(FindBone(iBoneID,piBoneIndex,&pmi->mi_cmiChildren[imich],iSkeletonLod)) 
        return TRUE;
    return FALSE;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  INDEX BuildHierarchy(CModelInstance *pmiModel, INDEX irmParent)
  {
    INDEX ctrm = _aRenModels.Count();
    RenModel &rm = _aRenModels.Push();
    RenModel &rmParent = _aRenModels[irmParent];

    rm.rm_pmiModel = pmiModel;
    rm.rm_iParentModelIndex = irmParent;
    rm.rm_iNextSiblingModel = -1;
    rm.rm_iFirstBone = _aRenBones.Count();
    rm.rm_ctBones = 0;

    if(pmiModel->mi_iParentBoneID == (-1)) {
      rm.rm_iParentBoneIndex = rmParent.rm_iFirstBone;
    } else {
      INDEX iParentBoneIndex = -1;
      if(rmParent.rm_pmiModel->mi_psklSkeleton != NULL && rmParent.rm_iSkeletonLODIndex>=0)  {
        iParentBoneIndex = rmParent.rm_pmiModel->mi_psklSkeleton->FindBoneInLOD(pmiModel->mi_iParentBoneID,rmParent.rm_iSkeletonLODIndex);
      } else {
        _aRenModels.Pop();
        return -1;
      }
      if(iParentBoneIndex == (-1)) {
        _aRenModels.Pop();
        return -1;
      } else {
        rm.rm_iParentBoneIndex = iParentBoneIndex + rmParent.rm_iFirstBone;
      }
    }
 
    if(pmiModel->mi_psklSkeleton!=NULL) {
      rm.rm_iSkeletonLODIndex = GetSkeletonLOD(*pmiModel->mi_psklSkeleton);
      if(rm.rm_iSkeletonLODIndex > -1) {
        INDEX ctsb = pmiModel->mi_psklSkeleton->skl_aSkeletonLODs[rm.rm_iSkeletonLODIndex].slod_aBones.Count();
        for(INDEX irb=0;irb<ctsb;irb++) {
          SkeletonBone *pSkeletonBone = &pmiModel->mi_psklSkeleton->skl_aSkeletonLODs[rm.rm_iSkeletonLODIndex].slod_aBones[irb];
          RenBone &rb = _aRenBones.Push();
          rb.rb_psbBone = pSkeletonBone;
          rb.rb_iRenModelIndex = ctrm;
          rm.rm_ctBones++;
          rb.rb_apPos.ap_vPos = pSkeletonBone->sb_qvRelPlacement.vPos;
          rb.rb_arRot.ar_qRot = pSkeletonBone->sb_qvRelPlacement.qRot;

          if(pSkeletonBone->sb_iParentID == (-1)) {
            rb.rb_iParentIndex = rm.rm_iParentBoneIndex;
          } else {
            INDEX rb_iParentIndex = pmiModel->mi_psklSkeleton->FindBoneInLOD(pSkeletonBone->sb_iParentID,rm.rm_iSkeletonLODIndex);
            rb.rb_iParentIndex = rb_iParentIndex + rm.rm_iFirstBone;
          }
        }
      }
    }
  
    rm.rm_iFirstMesh = _aRenMesh.Count();
    rm.rm_ctMeshes = 0;

    INDEX ctm = pmiModel->mi_aMeshInst.Count();
    for(INDEX im=0;im<ctm;im++) {
      INDEX iMeshLodIndex = GetMeshLOD(*pmiModel->mi_aMeshInst[im].mi_pMesh);
      if(iMeshLodIndex > -1) {
        RenMesh &rmsh = _aRenMesh.Push();
        rm.rm_ctMeshes++;
        rmsh.rmsh_iRenModelIndex = ctrm;
        rmsh.rmsh_pMeshInst = &pmiModel->mi_aMeshInst[im];
        rmsh.rmsh_iFirstMorph = _aRenMorph.Count();
        rmsh.rmsh_iFirstWeight = _aRenWeights.Count();
        rmsh.rmsh_ctMorphs = 0;
        rmsh.rmsh_ctWeights = 0;
        rmsh.rmsh_bTransToViewSpace = FALSE;
        rmsh.rmsh_iMeshLODIndex = iMeshLodIndex;

        INDEX ctmm = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aMorphMaps.Count();
        for(INDEX imm=0;imm<ctmm;imm++) {
          RenMorph &rm = _aRenMorph.Push();
          rmsh.rmsh_ctMorphs++;
          rm.rmp_pmmmMorphMap = &rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aMorphMaps[imm];
          rm.rmp_fFactor = 0;
        }

        INDEX ctw = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aWeightMaps.Count();
        for(INDEX iw=0;iw<ctw;iw++) {
          RenWeight &rw = _aRenWeights.Push();
          MeshWeightMap &mwm = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex].mlod_aWeightMaps[iw];
          rw.rw_pwmWeightMap = &mwm;
          rmsh.rmsh_ctWeights++;
          rw.rw_iBoneIndex = rm.rm_iFirstBone;
          if(!FindBone(mwm.mwm_iID,&rw.rw_iBoneIndex,pmiModel,rm.rm_iSkeletonLODIndex))
            rw.rw_iBoneIndex = -1;
        }
      }
    }

    rm.rm_iFirstChildModel = -1;
    INDEX ctmich = pmiModel->mi_cmiChildren.Count();
    for(int imich=0;imich<ctmich;imich++) {
      INDEX irmChildIndex = BuildHierarchy(&pmiModel->mi_cmiChildren[imich],ctrm);
      if(irmChildIndex != (-1)) {
        _aRenModels[irmChildIndex].rm_iNextSiblingModel = rm.rm_iFirstChildModel;
        rm.rm_iFirstChildModel = irmChildIndex;
      }
    }
    return ctrm;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  BOOL FindRenBone(RenModel &rm,int iBoneID,INDEX *piBoneIndex)
  {
    int ctb = rm.rm_iFirstBone + rm.rm_ctBones;
    for(int ib=rm.rm_iFirstBone;ib<ctb;ib++) {
      if(iBoneID == _aRenBones[ib].rb_psbBone->sb_iID) {
        *piBoneIndex = ib;
        return TRUE;
      }
    }
    return FALSE;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  INDEX FindFrame(UBYTE *pFirstMember, INDEX iFind, INDEX ctfn, UINT uiSize)
  {
    INDEX iHigh = ctfn-1;
    INDEX iLow = 0;
    INDEX iMid;

    UWORD iHighFrameNum = *(UWORD*)(pFirstMember+(uiSize*iHigh));
    if(iFind == iHighFrameNum) return iHigh;

    while(TRUE) {
      iMid = (iHigh+iLow)/2;
      UWORD iMidFrameNum = *(UWORD*)(pFirstMember+(uiSize*iMid));
      UWORD iMidFrameNumPlusOne = *(UWORD*)(pFirstMember+(uiSize*(iMid+1)));
      if(iFind < iMidFrameNum) iHigh = iMid;
      else if((iMid == iHigh) || (iMidFrameNumPlusOne > iFind)) return iMid;
      else iLow = iMid;
    }
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void DecompressAxis(FLOAT3D &vNormal, UWORD ubH, UWORD ubP)
  {
    ANGLE h = (ubH/65535.0f)*360.0f-180.0f;
    ANGLE p = (ubP/65535.0f)*360.0f-180.0f;

    FLOAT &x = vNormal(1);
    FLOAT &y = vNormal(2);
    FLOAT &z = vNormal(3);

    x = -Sin(h)*Cos(p);
    y = Sin(p);
    z = -Cos(h)*Cos(p);
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  BOOL FindRenMorph(RenModel &rm,int iMorphID,INDEX *piMorphIndex)
  {
    INDEX ctmsh = rm.rm_iFirstMesh + rm.rm_ctMeshes;
    for(INDEX irmsh=rm.rm_iFirstMesh;irmsh<ctmsh;irmsh++) {
      INDEX ctmm = _aRenMesh[irmsh].rmsh_iFirstMorph + _aRenMesh[irmsh].rmsh_ctMorphs;
      for(INDEX imm=_aRenMesh[irmsh].rmsh_iFirstMorph;imm<ctmm;imm++) {
        if(iMorphID == _aRenMorph[imm].rmp_pmmmMorphMap->mmp_iID) {
          *piMorphIndex = imm;
          return TRUE;
        }
      }
    }
    return FALSE;
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void MatchAnims(RenModel &rm)
  {
    const FLOAT fLerpedTick = _pTimer->GetLerpedCurrentTick();

    INDEX ctas = rm.rm_pmiModel->mi_aAnimSet.Count();
    if(ctas == 0) return;
    INDEX ctal = rm.rm_pmiModel->mi_aqAnims.aq_Lists.Count();
    INDEX iFirstAnimList = 0;
    INDEX ial=ctal-1;
    for(;ial>=0;ial--) {
      AnimList &alList = rm.rm_pmiModel->mi_aqAnims.aq_Lists[ial];
      FLOAT fFadeFactor = CalculateFadeFactor(alList);
      if(fFadeFactor >= 1.0f) {
        iFirstAnimList = ial;
        break;
      }
    }

    for(ial=iFirstAnimList;ial<ctal;ial++) {
      AnimList &alList = rm.rm_pmiModel->mi_aqAnims.aq_Lists[ial];
      AnimList *palListNext=NULL;
      if(ial+1<ctal) palListNext = &rm.rm_pmiModel->mi_aqAnims.aq_Lists[ial+1];
    
      FLOAT fFadeFactor = CalculateFadeFactor(alList);

      INDEX ctpa = alList.al_PlayedAnims.Count();
      for(int ipa=0;ipa<ctpa;ipa++) {
        FLOAT fTime = fLerpedTick;
        PlayedAnim &pa = alList.al_PlayedAnims[ipa];
        BOOL bAnimLooping = pa.pa_ulFlags & AN_LOOPING;

        INDEX iAnimSetIndex;
        INDEX iAnimIndex;
        if(rm.rm_pmiModel->FindAnimationByID(pa.pa_iAnimID,&iAnimSetIndex,&iAnimIndex)) {
          Animation &an = rm.rm_pmiModel->mi_aAnimSet[iAnimSetIndex].as_Anims[iAnimIndex];
        
          FLOAT fFadeInEndTime = alList.al_fStartTime + alList.al_fFadeTime;

          if(palListNext!=NULL) {
            fTime = ClampUp(fTime, palListNext->al_fStartTime);
          }

          FLOAT fTimeOffset = fTime - pa.pa_fStartTime;
          if (fLerpedTick < fFadeInEndTime) {
            fTimeOffset += fFadeInEndTime - fLerpedTick;
          }

          FLOAT f = fTimeOffset / (an.an_fSecPerFrame*pa.pa_fSpeedMul);

          INDEX iCurentFrame;
          INDEX iAnimFrame,iNextAnimFrame;
        
          if(bAnimLooping) {
            f = fmod(f,an.an_iFrames);
            iCurentFrame = INDEX(f);
            iAnimFrame = iCurentFrame % an.an_iFrames;
            iNextAnimFrame = (iCurentFrame+1) % an.an_iFrames;
          } else {
            if(f>an.an_iFrames) f = an.an_iFrames-1;
            iCurentFrame = INDEX(f);
            iAnimFrame = ClampUp(iCurentFrame,an.an_iFrames-1L);
            iNextAnimFrame = ClampUp(iCurentFrame+1L,an.an_iFrames-1L);
          }
        
          INDEX ctbe = an.an_abeBones.Count();
          for(int ibe=0;ibe<ctbe;ibe++) {
            INDEX iBoneIndex;
            if(FindRenBone(rm,an.an_abeBones[ibe].be_iBoneID, &iBoneIndex)) {
              RenBone &rb = _aRenBones[iBoneIndex];
              BoneEnvelope &be = an.an_abeBones[ibe];

              INDEX iRotFrameIndex;
              INDEX iNextRotFrameIndex;
              INDEX iRotFrameNum;
              INDEX iNextRotFrameNum;
              FLOAT fSlerpFactor;
              FLOATquat3D qRot;
              FLOATquat3D qRotCurrent;
              FLOATquat3D qRotNext;
              FLOATquat3D *pqRotCurrent;
              FLOATquat3D *pqRotNext;
            
              if(!an.an_bCompresed) {
                AnimRot *arFirst = &be.be_arRot[0];
                INDEX ctfn = be.be_arRot.Count();
                iRotFrameIndex = FindFrame((UBYTE*)arFirst,iAnimFrame,ctfn,sizeof(AnimRot));
              
                if(bAnimLooping) {
                  iNextRotFrameIndex = (iRotFrameIndex+1) % be.be_arRot.Count();
                } else {
                  iNextRotFrameIndex = ClampUp(iRotFrameIndex+1L,be.be_arRot.Count() - 1L);
                }
              
                iRotFrameNum = be.be_arRot[iRotFrameIndex].ar_iFrameNum;
                iNextRotFrameNum = be.be_arRot[iNextRotFrameIndex].ar_iFrameNum;
                pqRotCurrent = &be.be_arRot[iRotFrameIndex].ar_qRot;
                pqRotNext = &be.be_arRot[iNextRotFrameIndex].ar_qRot;
              } else {
                AnimRotOpt *aroFirst = &be.be_arRotOpt[0];
                INDEX ctfn = be.be_arRotOpt.Count();
                iRotFrameIndex = FindFrame((UBYTE*)aroFirst,iAnimFrame,ctfn,sizeof(AnimRotOpt));

                if(bAnimLooping) { 
                  iNextRotFrameIndex = (iRotFrameIndex+1L) % be.be_arRotOpt.Count();
                } else {
                  iNextRotFrameIndex = ClampUp(iRotFrameIndex+1L,be.be_arRotOpt.Count() - 1L);
                }

                AnimRotOpt &aroRot = be.be_arRotOpt[iRotFrameIndex];
                AnimRotOpt &aroRotNext = be.be_arRotOpt[iNextRotFrameIndex];
                iRotFrameNum = aroRot.aro_iFrameNum;
                iNextRotFrameNum = aroRotNext.aro_iFrameNum;
                FLOAT3D vAxis;
                ANGLE aAngle;

                aAngle = aroRot.aro_aAngle / ANG_COMPRESIONMUL;
                DecompressAxis(vAxis,aroRot.aro_ubH,aroRot.aro_ubP);
                qRotCurrent.FromAxisAngle(vAxis,aAngle);

                aAngle = aroRotNext.aro_aAngle / ANG_COMPRESIONMUL;
                DecompressAxis(vAxis,aroRotNext.aro_ubH,aroRotNext.aro_ubP);
                qRotNext.FromAxisAngle(vAxis,aAngle);
                pqRotCurrent = &qRotCurrent;
                pqRotNext = &qRotNext;
              }

              if(iNextRotFrameNum<=iRotFrameNum) {
                fSlerpFactor = (f-iRotFrameNum) / (an.an_iFrames-iRotFrameNum);
              } else {
                fSlerpFactor = (f-iRotFrameNum) / (iNextRotFrameNum-iRotFrameNum);
              }
            
              qRot = Slerp<FLOAT>(fSlerpFactor,*pqRotCurrent,*pqRotNext);
              rb.rb_arRot.ar_qRot = Slerp<FLOAT>(fFadeFactor*pa.pa_Strength,rb.rb_arRot.ar_qRot,qRot);

              AnimPos *apFirst = &be.be_apPos[0];
              INDEX ctfn = be.be_apPos.Count();
              INDEX iPosFrameIndex = FindFrame((UBYTE*)apFirst,iAnimFrame,ctfn,sizeof(AnimPos));

              INDEX iNextPosFrameIndex;
              if(bAnimLooping) { 
                iNextPosFrameIndex = (iPosFrameIndex+1) % be.be_apPos.Count();
              } else {
                iNextPosFrameIndex = ClampUp(iPosFrameIndex+1L,be.be_apPos.Count()-1L);
              }

              INDEX iPosFrameNum = be.be_apPos[iPosFrameIndex].ap_iFrameNum;
              INDEX iNextPosFrameNum = be.be_apPos[iNextPosFrameIndex].ap_iFrameNum;

              FLOAT fLerpFactor;
              if(iNextPosFrameNum<=iPosFrameNum) fLerpFactor = (f-iPosFrameNum) / (an.an_iFrames-iPosFrameNum);
              else fLerpFactor = (f-iPosFrameNum) / (iNextPosFrameNum-iPosFrameNum);
            
              FLOAT3D vPos;
              FLOAT3D vBonePosCurrent = be.be_apPos[iPosFrameIndex].ap_vPos;
              FLOAT3D vBonePosNext = be.be_apPos[iNextPosFrameIndex].ap_vPos;

              if((be.be_OffSetLen > 0) && (rb.rb_psbBone->sb_fOffSetLen > 0)) {
                vBonePosCurrent *= (rb.rb_psbBone->sb_fOffSetLen / be.be_OffSetLen);
                vBonePosNext *= (rb.rb_psbBone->sb_fOffSetLen / be.be_OffSetLen);
              }

              vPos = Lerp(vBonePosCurrent,vBonePosNext,fLerpFactor);
              rb.rb_apPos.ap_vPos = Lerp(rb.rb_apPos.ap_vPos,vPos,fFadeFactor * pa.pa_Strength);
            }
          }

          for(INDEX im=0;im<an.an_ameMorphs.Count();im++) {
            INDEX iMorphIndex;
            if(FindRenMorph(rm,an.an_ameMorphs[im].me_iMorphMapID,&iMorphIndex)) {
              FLOAT &fCurFactor = an.an_ameMorphs[im].me_aFactors[iAnimFrame];
              FLOAT &fLastFactor = an.an_ameMorphs[im].me_aFactors[iNextAnimFrame];
              FLOAT fFactor = Lerp(fCurFactor,fLastFactor,f-iAnimFrame);

              _aRenMorph[iMorphIndex].rmp_fFactor = Lerp(_aRenMorph[iMorphIndex].rmp_fFactor,
                                                        fFactor,
                                                        fFadeFactor * pa.pa_Strength);
            }
          }
        }
      }
    }
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void CalculateBoneTransforms()
  {
    MatrixCopy(_aRenBones[0].rb_mTransform, _mObjToView);
    MatrixCopy(_aRenBones[0].rb_mStrTransform, _aRenBones[0].rb_mTransform);

    Matrix12 mStretch;
    int irb=1;
    for(; irb<_aRenBones.Count(); irb++) {
      Matrix12 mRelPlacement;
      Matrix12 mOffset;
      RenBone &rb = _aRenBones[irb];
      RenBone &rbParent = _aRenBones[rb.rb_iParentIndex];
      QVect qv;
      qv.vPos = rb.rb_apPos.ap_vPos;
      qv.qRot = rb.rb_arRot.ar_qRot;
      QVectToMatrix12(mRelPlacement,qv);

      if(rb.rb_psbBone->sb_iParentID == (-1)) {
        RenModel &rm= _aRenModels[rb.rb_iRenModelIndex];
        MakeStretchMatrix(mStretch, rm.rm_pmiModel->mi_vStretch);
      

        RenModel &rmParent = _aRenModels[rb.rb_iRenModelIndex];
        QVectToMatrix12(mOffset,rmParent.rm_pmiModel->mi_qvOffset);
        MatrixMultiplyCP(mRelPlacement,mOffset,mRelPlacement);

        Matrix12 mStrParentBoneTrans;
        MatrixMultiplyCP(mStrParentBoneTrans, rbParent.rb_mStrTransform,mStretch);
        MatrixMultiply(rb.rb_mStrTransform, mStrParentBoneTrans, mRelPlacement);
        MatrixMultiply(rb.rb_mTransform,rbParent.rb_mTransform, mRelPlacement);
      } else {
        MatrixMultiply(rb.rb_mStrTransform, rbParent.rb_mStrTransform, mRelPlacement);
        MatrixMultiply(rb.rb_mTransform,rbParent.rb_mTransform,mRelPlacement);
      }
      MatrixCopy(rb.rb_mBonePlacement,rb.rb_mStrTransform);
    }

    for(int irm=1; irm<_aRenModels.Count(); irm++) {
      Matrix12 mOffset;
      Matrix12 mStretch;
      RenModel &rm = _aRenModels[irm];

      QVectToMatrix12(mOffset,rm.rm_pmiModel->mi_qvOffset);
      MakeStretchMatrix(mStretch,rm.rm_pmiModel->mi_vStretch);

      MatrixMultiply(rm.rm_mTransform,_aRenBones[rm.rm_iParentBoneIndex].rb_mTransform,mOffset);
      MatrixMultiply(rm.rm_mStrTransform,_aRenBones[rm.rm_iParentBoneIndex].rb_mStrTransform,mOffset);
      MatrixMultiplyCP(rm.rm_mStrTransform,rm.rm_mStrTransform,mStretch);
    }

    Matrix12 mInvert;
    for(irb=1; irb<_aRenBones.Count(); irb++) {
      RenBone &rb = _aRenBones[irb];
      MatrixTranspose(mInvert,rb.rb_psbBone->sb_mAbsPlacement);
      MatrixMultiplyCP(_aRenBones[irb].rb_mStrTransform,_aRenBones[irb].rb_mStrTransform,mInvert);
      MatrixMultiplyCP(_aRenBones[irb].rb_mTransform,_aRenBones[irb].rb_mTransform,mInvert);
    }
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void CalculateRenderingData(CModelInstance &mi)
  {
    SetObjectMatrices(mi);

    MakeRootModel();
    BuildHierarchy(&mi, 0);

    INDEX ctrm = _aRenModels.Count();
    for(int irm=1;irm<ctrm;irm++)
      MatchAnims(_aRenModels[irm]);

    CalculateBoneTransforms();
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void PrepareMeshForRendering(RenMesh& rmsh, INDEX iSkeletonlod)
  {
    MeshLOD &mlod = rmsh.rmsh_pMeshInst->mi_pMesh->msh_aMeshLODs[rmsh.rmsh_iMeshLODIndex];
    _aMorphedVtxs.PopAll();
    _aFinalVtxs.PopAll();
    _pavFinalVertices = NULL;

    INDEX ctVertices = mlod.mlod_aVertices.Count();
    _aMorphedVtxs.Push(ctVertices);
    _aFinalVtxs.Push(ctVertices);
    _ctFinalVertices = ctVertices;
  
    memcpy(&_aMorphedVtxs[0],&mlod.mlod_aVertices[0],sizeof(mlod.mlod_aVertices[0]) * ctVertices);
    memset(&_aFinalVtxs[0],0,sizeof(_aFinalVtxs[0])*ctVertices);

    INDEX ctmm = rmsh.rmsh_iFirstMorph + rmsh.rmsh_ctMorphs;
    for(int irm=rmsh.rmsh_iFirstMorph;irm<ctmm;irm++)
    {
      RenMorph &rm = _aRenMorph[irm];
      if(rm.rmp_fFactor > 0.0f) {
        for(int ivx=0;ivx<rm.rmp_pmmmMorphMap->mmp_aMorphMap.Count();ivx++) {
          if(rm.rmp_pmmmMorphMap->mmp_bRelative) {
            INDEX vtx = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx].mwm_iVxIndex;
            MeshVertex &mvSrc = mlod.mlod_aVertices[vtx];
            MeshNormal &mnSrc = mlod.mlod_aNormals[vtx];
            MeshVertexMorph &mvmDst = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx];
            _aMorphedVtxs[vtx].x += rm.rmp_fFactor*(mvmDst.mwm_x - mvSrc.x);
            _aMorphedVtxs[vtx].y += rm.rmp_fFactor*(mvmDst.mwm_y - mvSrc.y);
            _aMorphedVtxs[vtx].z += rm.rmp_fFactor*(mvmDst.mwm_z - mvSrc.z);
          } else {
            INDEX vtx = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx].mwm_iVxIndex;
            MeshVertexMorph &mvmDst = rm.rmp_pmmmMorphMap->mmp_aMorphMap[ivx];
            _aMorphedVtxs[vtx].x = (1.0f-rm.rmp_fFactor) * _aMorphedVtxs[vtx].x + rm.rmp_fFactor*mvmDst.mwm_x;
            _aMorphedVtxs[vtx].y = (1.0f-rm.rmp_fFactor) * _aMorphedVtxs[vtx].y + rm.rmp_fFactor*mvmDst.mwm_y;
            _aMorphedVtxs[vtx].z = (1.0f-rm.rmp_fFactor) * _aMorphedVtxs[vtx].z + rm.rmp_fFactor*mvmDst.mwm_z;
          }
        }
      }
    }

    INDEX ctrw = rmsh.rmsh_iFirstWeight + rmsh.rmsh_ctWeights;
    INDEX ctbones = 0;
    CSkeleton *pskl = _aRenModels[rmsh.rmsh_iRenModelIndex].rm_pmiModel->mi_psklSkeleton;
    if((pskl!=NULL) && (iSkeletonlod > -1))
      ctbones = pskl->skl_aSkeletonLODs[iSkeletonlod].slod_aBones.Count();

    if(ctbones > 0 && ctrw>0) {
      for(int irw=rmsh.rmsh_iFirstWeight; irw<ctrw; irw++) {
        RenWeight &rw = _aRenWeights[irw];
        Matrix12 mTransform;
        Matrix12 mStrTransform;
        if(rw.rw_iBoneIndex == (-1)) {
          MatrixCopy(mStrTransform, _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mStrTransform);
          MatrixCopy(mTransform,    _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mTransform);
        } else {
          MatrixCopy(mStrTransform, _aRenBones[rw.rw_iBoneIndex].rb_mStrTransform);
          MatrixCopy(mTransform,    _aRenBones[rw.rw_iBoneIndex].rb_mTransform);
        }

        if(mlod.mlod_ulFlags & ML_FULL_FACE_FORWARD)
          RemoveRotationFromMatrix(mStrTransform);

        INDEX ctvw = rw.rw_pwmWeightMap->mwm_aVertexWeight.Count();
        for(int ivw=0; ivw<ctvw; ivw++) {
          MeshVertexWeight &vw = rw.rw_pwmWeightMap->mwm_aVertexWeight[ivw];
          INDEX ivx = vw.mww_iVertex;
          MeshVertex mv = _aMorphedVtxs[ivx];
        
          TransformVector((FLOAT3&)mv,mStrTransform);

          _aFinalVtxs[ivx].x += mv.x * vw.mww_fWeight;
          _aFinalVtxs[ivx].y += mv.y * vw.mww_fWeight;
          _aFinalVtxs[ivx].z += mv.z * vw.mww_fWeight;
        }
      }
      _pavFinalVertices = &_aFinalVtxs[0];
      rmsh.rmsh_bTransToViewSpace = TRUE;
    } else {
      Matrix12 mTransform;
      Matrix12 mStrTransform;
      MatrixCopy(mTransform,    _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mTransform);
      MatrixCopy(mStrTransform, _aRenModels[rmsh.rmsh_iRenModelIndex].rm_mStrTransform);

      if(mlod.mlod_ulFlags & ML_FULL_FACE_FORWARD)
        RemoveRotationFromMatrix(mStrTransform);

      for(int ivx=0;ivx<ctVertices;ivx++) {
        MeshVertex &mv = _aMorphedVtxs[ivx];
        TransformVector((FLOAT3&)mv,mStrTransform);
        _aFinalVtxs[ivx].x = mv.x;
        _aFinalVtxs[ivx].y = mv.y;
        _aFinalVtxs[ivx].z = mv.z;
      }
      _pavFinalVertices = &_aFinalVtxs[0];
      rmsh.rmsh_bTransToViewSpace = TRUE;
    }
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void UnpackVertex_CU(const INDEX iVertex, FLOAT3D& vVertex)
  {
    if (_b16Bit) {
      const SWPOINT3D &vsw0 = _pFrame16_0[iVertex].mfv_SWPoint;
      const SWPOINT3D &vsw1 = _pFrame16_1[iVertex].mfv_SWPoint;
      vVertex(1) = (Lerp( (FLOAT)vsw0(1), (FLOAT)vsw1(1), _fRatio) -_vOffset(1)) * _vStretch(1);
      vVertex(2) = (Lerp( (FLOAT)vsw0(2), (FLOAT)vsw1(2), _fRatio) -_vOffset(2)) * _vStretch(2);
      vVertex(3) = (Lerp( (FLOAT)vsw0(3), (FLOAT)vsw1(3), _fRatio) -_vOffset(3)) * _vStretch(3);
    } else {
      const SBPOINT3D &vsb0 = _pFrame8_0[iVertex].mfv_SBPoint;
      const SBPOINT3D &vsb1 = _pFrame8_1[iVertex].mfv_SBPoint;
      vVertex(1) = (Lerp( (FLOAT)vsb0(1), (FLOAT)vsb1(1), _fRatio) -_vOffset(1)) * _vStretch(1);
      vVertex(2) = (Lerp( (FLOAT)vsb0(2), (FLOAT)vsb1(2), _fRatio) -_vOffset(2)) * _vStretch(2);
      vVertex(3) = (Lerp( (FLOAT)vsb0(3), (FLOAT)vsb1(3), _fRatio) -_vOffset(3)) * _vStretch(3);
    }
  }

//------------------------------------------------------------------------------------------------------------------------------------------
  void GetAttachmentMatrices_CU(CModelObject& mo, CAttachmentModelObject* pamo, FLOATmatrix3D& mRotation, FLOAT3D& vPosition)
  {
    CModelData* pmdMain = (CModelData *)mo.GetData();
    pmdMain->md_aampAttachedPosition.Lock();
    const CAttachedModelPosition &amp = pmdMain->md_aampAttachedPosition[pamo->amo_iAttachedPosition];
    pmdMain->md_aampAttachedPosition.Unlock();

    FLOAT3D &vDataStretch = pmdMain->md_Stretch;
    FLOAT3D &vObjectStretch = mo.mo_Stretch;
    _vStretch(1) = vDataStretch(1)*vObjectStretch(1);
    _vStretch(2) = vDataStretch(2)*vObjectStretch(2);
    _vStretch(3) = vDataStretch(3)*vObjectStretch(3);
    _vOffset = pmdMain->md_vCompressedCenter;

    INDEX iFrame0, iFrame1;
    mo.GetFrame( iFrame0, iFrame1, _fRatio);
    const INDEX ctVertices = pmdMain->md_VerticesCt;
    if( pmdMain->md_Flags & MF_COMPRESSED_16BIT) {
      _b16Bit = TRUE;
      _pFrame16_0 = &pmdMain->md_FrameVertices16[iFrame0 *ctVertices];
      _pFrame16_1 = &pmdMain->md_FrameVertices16[iFrame1 *ctVertices];
    } else {
      _b16Bit = FALSE;
      _pFrame8_0 = &pmdMain->md_FrameVertices8[iFrame0 *ctVertices];
      _pFrame8_1 = &pmdMain->md_FrameVertices8[iFrame1 *ctVertices];
    }

    FLOAT3D vCenter, vFront, vUp;
    const INDEX iCenter = amp.amp_iCenterVertex;
    const INDEX iFront  = amp.amp_iFrontVertex;
    const INDEX iUp     = amp.amp_iUpVertex;
    UnpackVertex_CU( iCenter, vCenter);
    UnpackVertex_CU( iFront,  vFront);
    UnpackVertex_CU( iUp,     vUp);

    FLOAT3D vY = vUp - vCenter;
    FLOAT3D vZ = vCenter - vFront;
    const FLOATmatrix3D &mO2A = mRotation;
    const FLOAT3D &vO2A = vPosition;
    vCenter = vCenter*mO2A +vO2A;
    vY = vY *mO2A;
    vZ = vZ *mO2A;

    FLOAT3D vX = vY*vZ;
    vY = vZ*vX;
    vX.Normalize();
    vY.Normalize();
    vZ.Normalize();
    FLOATmatrix3D mOrientation;
    mOrientation(1,1) = vX(1);  mOrientation(1,2) = vY(1);  mOrientation(1,3) = vZ(1);
    mOrientation(2,1) = vX(2);  mOrientation(2,2) = vY(2);  mOrientation(2,3) = vZ(2);
    mOrientation(3,1) = vX(3);  mOrientation(3,2) = vY(3);  mOrientation(3,3) = vZ(3);

    FLOAT3D vOffset;
    FLOATmatrix3D mRelative;
    MakeRotationMatrixFast( mRelative, pamo->amo_plRelative.pl_OrientationAngle);
    vOffset(1) = pamo->amo_plRelative.pl_PositionVector(1) * mo.mo_Stretch(1);
    vOffset(2) = pamo->amo_plRelative.pl_PositionVector(2) * mo.mo_Stretch(2);
    vOffset(3) = pamo->amo_plRelative.pl_PositionVector(3) * mo.mo_Stretch(3);
    const FLOAT3D vO = vCenter + vOffset * mOrientation;
    mRotation = mOrientation*mRelative;
    vPosition = vO;
  }
} // anonymous namespace
