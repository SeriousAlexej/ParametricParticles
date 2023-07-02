#include <Engine/Engine.h>

#define DECL_DLL __declspec(dllimport)
#include "EntitiesMP/Global.h"
#undef DECL_DLL
#define DECL_DLL __declspec(dllexport)
#include "ParametricParticlesMP/WeakPointer.h"

bool InWED();

#define ID_ParametricParticles 4242
#define ID_SpawnShapeBox       4243
#define ID_SpawnShapeSphere    4244
#define ID_SpawnShapeCylinder  4245
#define ID_ParticleRotation    4246
#define ID_ParticleVelocity    4247
#define ID_AutoHeightMap       4248
#define ID_SpawnShapeBase      4249

#define ENTITY_ID(entity) entity->GetClass()->ec_pdecDLLClass->dec_iID
#define ANCESTOR_ID(entity) entity->GetClass()->ec_pdecDLLClass->dec_pdecBase->dec_iID
