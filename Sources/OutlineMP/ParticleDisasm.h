#ifndef PARTICLE_DISASM_H
#define PARTICLE_DISASM_H

void Particle_RenderQuad3D_NEW(
  const FLOAT3D& vPos0,
  const FLOAT3D& vPos1,
  const FLOAT3D& vPos2,
  const FLOAT3D& vPos3,
  const FLOAT2D& uv0,
  const FLOAT2D& uv1,
  const FLOAT2D& uv2,
  const FLOAT2D& uv3,
  COLOR col);

void Particle_RenderQuad3D_NEW(
  const FLOAT3D& vPos0,
  const FLOAT3D& vPos1,
  const FLOAT3D& vPos2,
  const FLOAT3D& vPos3,
  FLOAT uv_end,
  COLOR col);

#endif
