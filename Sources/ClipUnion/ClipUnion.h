#ifndef CLIP_UNION_H
#define CLIP_UNION_H

#ifdef CLIP_UNION_EXPORTS
#define CLIP_UNION_API __declspec(dllexport)
#else
#define CLIP_UNION_API __declspec(dllimport)
#endif

extern "C"
{
  CLIP_UNION_API void __cdecl CU_Clear();
  CLIP_UNION_API void __cdecl CU_AddPolygon(float* p_points, unsigned int numPoints);
  CLIP_UNION_API unsigned int __cdecl CU_UnitePolygons();
  CLIP_UNION_API unsigned int __cdecl CU_GetContour(unsigned int contourIndex, float** p_points);
}

#endif // CLIP_UNION_H
