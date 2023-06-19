#include <Engine/Engine.h>

#define DECL_DLL __declspec(dllimport)
#include "EntitiesMP/Global.h"
#undef DECL_DLL
#define DECL_DLL __declspec(dllexport)