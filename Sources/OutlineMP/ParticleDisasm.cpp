#include <Engine/Engine.h>
#include <ParametricParticlesUtils/ParticlesUtils.h>
#include "ParticleDisasm.h"

ULONG GetFogAlpha(const GFXTexCoord &tex)
{
  static const int enginedll_base_address = reinterpret_cast<int>(&CEntity::HandleSentEvents) - 0xFEC80;

  static PIX*    _fog_pixSizeH_DISASM = reinterpret_cast<PIX*>   (enginedll_base_address + 0x1F86EC);
  static PIX*    _fog_pixSizeL_DISASM = reinterpret_cast<PIX*>   (enginedll_base_address + 0x1F86F0);
  static UBYTE** _fog_pubTable_DISASM = reinterpret_cast<UBYTE**>(enginedll_base_address + 0x1F86FC);
  static ULONG*  _fog_ulAlpha_DISASM  = reinterpret_cast<ULONG*> (enginedll_base_address + 0x1F86E0);

  // point sampling of height
  PIX pixT = FloatToInt( tex.t * (*_fog_pixSizeH_DISASM));
      pixT = Clamp( pixT, 0L, (*_fog_pixSizeH_DISASM)-1L) * (*_fog_pixSizeL_DISASM);
  // linear interpolation of depth
  const PIX pixSF = FloatToInt( tex.s*(FLOAT)(*_fog_pixSizeL_DISASM)*255.499f);
  const PIX pixS1 = Clamp( (PIX)((pixSF>>8)+0), 0L, (*_fog_pixSizeL_DISASM)-1L);
  const PIX pixS2 = Clamp( (PIX)((pixSF>>8)+1), 0L, (*_fog_pixSizeL_DISASM)-1L);
  const ULONG ulF  = pixSF & 255;
  const ULONG ulA1 = (*_fog_pubTable_DISASM)[pixT +pixS1];
  const ULONG ulA2 = (*_fog_pubTable_DISASM)[pixT +pixS2];
  return ((ulA1*(ulF^255)+ulA2*ulF) * (*_fog_ulAlpha_DISASM)) >>16;
}

ULONG GetHazeAlpha(const FLOAT fS)
{
  static const int enginedll_base_address = reinterpret_cast<int>(&CEntity::HandleSentEvents) - 0xFEC80;

  static PIX*    _haze_pixSize_DISASM  = reinterpret_cast<PIX*>   (enginedll_base_address + 0x1F8704);
  static UBYTE** _haze_pubTable_DISASM = reinterpret_cast<UBYTE**>(enginedll_base_address + 0x1F870C);
  static ULONG*  _haze_ulAlpha_DISASM  = reinterpret_cast<ULONG*> (enginedll_base_address + 0x1F8718);

  // linear interpolation of depth
  const PIX pixSH = FloatToInt( fS*(FLOAT)(*_haze_pixSize_DISASM)*255.4999f);
  const PIX pixS1 = Clamp( (PIX)((pixSH>>8)+0), 0L, (*_haze_pixSize_DISASM)-1L);
  const PIX pixS2 = Clamp( (PIX)((pixSH>>8)+1), 0L, (*_haze_pixSize_DISASM)-1L);
  const ULONG ulH  = pixSH & 255;
  const ULONG ulA1 = (*_haze_pubTable_DISASM)[pixS1];
  const ULONG ulA2 = (*_haze_pubTable_DISASM)[pixS2];
  return ((ulA1*(ulH^255)+ulA2*ulH) * (*_haze_ulAlpha_DISASM)) >>16;
}

void Particle_RenderQuad3D_NEW(
  const FLOAT3D& vPos0,
  const FLOAT3D& vPos1,
  const FLOAT3D& vPos2,
  const FLOAT3D& vPos3,
  const FLOAT2D& uv0,
  const FLOAT2D& uv1,
  const FLOAT2D& uv2,
  const FLOAT2D& uv3,
  COLOR col)
{
  static const int enginedll_base_address = reinterpret_cast<int>(&CEntity::HandleSentEvents) - 0xFEC80;
  
  static BOOL*                        _bNeedsClipping_DISASM = reinterpret_cast<BOOL*>                          (enginedll_base_address + 0x1F64E4);
  static CStaticStackArray<GFXTexCoord>* _atexFogHaze_DISASM = reinterpret_cast<CStaticStackArray<GFXTexCoord>*>(enginedll_base_address + 0x1F6478);
  static BOOL*                      _Particle_bHasFog_DISASM = reinterpret_cast<BOOL*>                          (enginedll_base_address + 0x1F64D8);
  static BOOL*                     _Particle_bHasHaze_DISASM = reinterpret_cast<BOOL*>                          (enginedll_base_address + 0x1F64DC);
  static BOOL*                         _bTransFogHaze_DISASM = reinterpret_cast<BOOL*>                          (enginedll_base_address + 0x1F64E0);
  static COLOR*                           _colAttMask_DISASM = reinterpret_cast<COLOR*>                         (enginedll_base_address + 0x1F648C);
  static SLONG*                      _slTexSaturation_DISASM = reinterpret_cast<SLONG*>                         (enginedll_base_address + 0x1E08B4);
  static SLONG*                        _slTexHueShift_DISASM = reinterpret_cast<SLONG*>                         (enginedll_base_address + 0x207294);
  static FLOAT*                            _haze_fAdd_DISASM = reinterpret_cast<FLOAT*>                         (enginedll_base_address + 0x1F8714);
  static FLOAT*                            _haze_fMul_DISASM = reinterpret_cast<FLOAT*>                         (enginedll_base_address + 0x1F8710);
  static FLOAT*                            _fog_fMulZ_DISASM = reinterpret_cast<FLOAT*>                         (enginedll_base_address + 0x1F86D4);
  static FLOAT3D*                      _fog_vHDirView_DISASM = reinterpret_cast<FLOAT3D*>                       (enginedll_base_address + 0x1F8638);
  static FLOAT*                            _fog_fAddH_DISASM = reinterpret_cast<FLOAT*>                         (enginedll_base_address + 0x1F86DC);
  static FLOAT*                            _fog_fMulH_DISASM = reinterpret_cast<FLOAT*>                         (enginedll_base_address + 0x1F86D8);
  static CStaticStackArray<GFXVertex>*    _avtxCommon_DISASM = reinterpret_cast<CStaticStackArray<GFXVertex>*>  (enginedll_base_address + 0x1F9DC0);
  static CStaticStackArray<GFXTexCoord>*  _atexCommon_DISASM = reinterpret_cast<CStaticStackArray<GFXTexCoord>*>(enginedll_base_address + 0x1F9DB0);
  static CStaticStackArray<GFXColor>*     _acolCommon_DISASM = reinterpret_cast<CStaticStackArray<GFXColor>*>   (enginedll_base_address + 0x1F9DA0);

  CProjection3D* _prProjection = Particle_GetProjection();

  // trivial rejection
  if( ((col&CT_AMASK)>>CT_ASHIFT)<2) return;

  // project point to screen
  FLOAT3D vProjected0, vProjected1, vProjected2, vProjected3;
  _prProjection->PreClip( vPos0, vProjected0);
  _prProjection->PreClip( vPos1, vProjected1);
  _prProjection->PreClip( vPos2, vProjected2);
  _prProjection->PreClip( vPos3, vProjected3);

  // test for trivial rejection (sphere method)
  FLOAT3D vNearest = vProjected0; // find nearest-Z vertex
  if( vNearest(3)>vProjected1(3)) vNearest = vProjected1;
  if( vNearest(3)>vProjected2(3)) vNearest = vProjected2;
  if( vNearest(3)>vProjected3(3)) vNearest = vProjected3;
  // find center
  const FLOAT fX = (vProjected0(1)+vProjected1(1)+vProjected2(1)+vProjected3(1)) * 0.25f;
  const FLOAT fY = (vProjected0(2)+vProjected1(2)+vProjected2(2)+vProjected3(2)) * 0.25f;
  // find radius (approx. distance to nearest-Z vertex)
  // we won't do sqrt but rather larger distance * 0.7f (1/sqrt(2))
  const FLOAT fDX = Abs(fX-vNearest(1));
  const FLOAT fDY = Abs(fY-vNearest(2));
  const FLOAT fR  = 0.7f * Max(fDX,fDY);
  // set center vertex location and test it
  vNearest(1) = fX;
  vNearest(2) = fY;
  const INDEX iTest = _prProjection->TestSphereToFrustum( vNearest, fR);

  // adjust the need for clipping
  if( iTest==0) (*_bNeedsClipping_DISASM) = TRUE;

  // separate colors (for the sake of fog/haze)
  COLOR col0,col1,col2,col3;
  col0 = col3 = col;
  col1 = col2 = col;
  // eventual tex coords for fog or haze
  const INDEX ctTexFG = (*_atexFogHaze_DISASM).Count();
  GFXTexCoord *ptexFogHaze = &(*_atexFogHaze_DISASM)[ctTexFG-4];

  // if haze is active
  if( (*_Particle_bHasHaze_DISASM))
  { // get haze strength at particle position
    ptexFogHaze[0].s = (-vProjected0(3)+(*_haze_fAdd_DISASM))*(*_haze_fMul_DISASM);
    ptexFogHaze[1].s = (-vProjected1(3)+(*_haze_fAdd_DISASM))*(*_haze_fMul_DISASM);
    ptexFogHaze[2].s = (-vProjected2(3)+(*_haze_fAdd_DISASM))*(*_haze_fMul_DISASM);
    ptexFogHaze[3].s = (-vProjected3(3)+(*_haze_fAdd_DISASM))*(*_haze_fMul_DISASM);
    const ULONG ulH0 = 255-GetHazeAlpha(ptexFogHaze[0].s);
    const ULONG ulH1 = 255-GetHazeAlpha(ptexFogHaze[1].s);
    const ULONG ulH2 = 255-GetHazeAlpha(ptexFogHaze[2].s);
    const ULONG ulH3 = 255-GetHazeAlpha(ptexFogHaze[3].s);
    if( (ulH0|ulH1|ulH2|ulH3)<4) return;
    if( (*_colAttMask_DISASM)) { // apply haze color (if not transparent)
      COLOR colH;
      colH = (*_colAttMask_DISASM) | RGBAToColor( ulH0,ulH0,ulH0,ulH0);  col0 = MulColors( col0, colH);
      colH = (*_colAttMask_DISASM) | RGBAToColor( ulH1,ulH1,ulH1,ulH1);  col1 = MulColors( col1, colH);
      colH = (*_colAttMask_DISASM) | RGBAToColor( ulH2,ulH2,ulH2,ulH2);  col2 = MulColors( col2, colH);
      colH = (*_colAttMask_DISASM) | RGBAToColor( ulH3,ulH3,ulH3,ulH3);  col3 = MulColors( col3, colH);
    } else ptexFogHaze[0].t = ptexFogHaze[1].t = ptexFogHaze[2].t = ptexFogHaze[3].t = 0;
  }
  // if fog is active
  if( (*_Particle_bHasFog_DISASM))
  { // get fog strength at particle position
    ptexFogHaze[0].s = -vProjected0(3)*(*_fog_fMulZ_DISASM);
    ptexFogHaze[0].t = (vProjected0%(*_fog_vHDirView_DISASM)+(*_fog_fAddH_DISASM))*(*_fog_fMulH_DISASM);
    ptexFogHaze[1].s = -vProjected1(3)*(*_fog_fMulZ_DISASM);
    ptexFogHaze[1].t = (vProjected1%(*_fog_vHDirView_DISASM)+(*_fog_fAddH_DISASM))*(*_fog_fMulH_DISASM);
    ptexFogHaze[2].s = -vProjected2(3)*(*_fog_fMulZ_DISASM);
    ptexFogHaze[2].t = (vProjected2%(*_fog_vHDirView_DISASM)+(*_fog_fAddH_DISASM))*(*_fog_fMulH_DISASM);
    ptexFogHaze[3].s = -vProjected3(3)*(*_fog_fMulZ_DISASM);
    ptexFogHaze[3].t = (vProjected3%(*_fog_vHDirView_DISASM)+(*_fog_fAddH_DISASM))*(*_fog_fMulH_DISASM);
    const ULONG ulF0 = 255-GetFogAlpha(ptexFogHaze[0]);
    const ULONG ulF1 = 255-GetFogAlpha(ptexFogHaze[1]);
    const ULONG ulF2 = 255-GetFogAlpha(ptexFogHaze[2]);
    const ULONG ulF3 = 255-GetFogAlpha(ptexFogHaze[3]);
    if( (ulF0|ulF1|ulF2|ulF3)<4) return;
    if( (*_colAttMask_DISASM)) { // apply fog color (if not transparent)
      COLOR colF; 
      colF = (*_colAttMask_DISASM) | RGBAToColor( ulF0,ulF0,ulF0,ulF0);  col0 = MulColors( col0, colF);
      colF = (*_colAttMask_DISASM) | RGBAToColor( ulF1,ulF1,ulF1,ulF1);  col1 = MulColors( col1, colF);
      colF = (*_colAttMask_DISASM) | RGBAToColor( ulF2,ulF2,ulF2,ulF2);  col2 = MulColors( col2, colF);
      colF = (*_colAttMask_DISASM) | RGBAToColor( ulF3,ulF3,ulF3,ulF3);  col3 = MulColors( col3, colF);
    }
  }
  // keep fog/haze tex coords (if needed)
  if( (*_bTransFogHaze_DISASM)) (*_atexFogHaze_DISASM).Push(4);

  // add to vertex arrays
  GFXVertex   *pvtx = (*_avtxCommon_DISASM).Push(4);
  GFXTexCoord *ptex = (*_atexCommon_DISASM).Push(4);
  GFXColor    *pcol = (*_acolCommon_DISASM).Push(4);

  // prepare vertices
  pvtx[0].x = vProjected0(1);  pvtx[0].y = vProjected0(2);  pvtx[0].z = vProjected0(3);
  pvtx[1].x = vProjected1(1);  pvtx[1].y = vProjected1(2);  pvtx[1].z = vProjected1(3);
  pvtx[2].x = vProjected2(1);  pvtx[2].y = vProjected2(2);  pvtx[2].z = vProjected2(3);
  pvtx[3].x = vProjected3(1);  pvtx[3].y = vProjected3(2);  pvtx[3].z = vProjected3(3);
  // prepare texture coords 
  ptex[0].u = uv0(1); ptex[0].v = uv0(2);
  ptex[1].u = uv1(1); ptex[1].v = uv1(2);
  ptex[2].u = uv2(1); ptex[2].v = uv2(2);
  ptex[3].u = uv3(1); ptex[3].v = uv3(2);
  // prepare colors
  const GFXColor glcol0( AdjustColor( col0, (*_slTexHueShift_DISASM), (*_slTexSaturation_DISASM)));
  const GFXColor glcol1( AdjustColor( col1, (*_slTexHueShift_DISASM), (*_slTexSaturation_DISASM)));
  const GFXColor glcol2( AdjustColor( col2, (*_slTexHueShift_DISASM), (*_slTexSaturation_DISASM)));
  const GFXColor glcol3( AdjustColor( col3, (*_slTexHueShift_DISASM), (*_slTexSaturation_DISASM)));
  pcol[0] = glcol0;
  pcol[1] = glcol1;
  pcol[2] = glcol2;
  pcol[3] = glcol3;
}

void Particle_RenderQuad3D_NEW(
  const FLOAT3D& vPos0,
  const FLOAT3D& vPos1,
  const FLOAT3D& vPos2,
  const FLOAT3D& vPos3,
  FLOAT uv_end,
  COLOR col)
{
  Particle_RenderQuad3D_NEW(
    vPos0,
    vPos1,
    vPos2,
    vPos3,
    FLOAT2D(0.0f, uv_end),
    FLOAT2D(0.0f, 0.01f),
    FLOAT2D(1.0f, 0.01f),
    FLOAT2D(1.0f, uv_end),
    col);
}