#include <Engine/Engine.h>

#define DECL_DLL __declspec(dllimport)
#include "EntitiesMP/Global.h"
#undef DECL_DLL
#define DECL_DLL __declspec(dllexport)
#include "ParametricParticlesMP/WeakPointer.h"

typedef EnableWeakPointer<CMovableModelEntity> CMovableModelEntity_EnableWeakPointer;
#define CMovableModelEntity_EnableWeakPointer_DLLClass CMovableModelEntity_DLLClass

typedef EnableWeakPointer<CEntity> CEntity_EnableWeakPointer;
#define CEntity_EnableWeakPointer_DLLClass CEntity_DLLClass
