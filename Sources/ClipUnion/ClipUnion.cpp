#include "ClipUnion.h"
#include <clipper2/clipper.h>

static std::vector<std::vector<float>> g_contours;
static Clipper2Lib::Paths64 g_polygons;

extern "C"
{
  void __cdecl CU_Clear()
  {
    g_contours.clear();
    g_polygons.clear();
  }

  void __cdecl CU_AddPolygon(float* p_points, unsigned int numPoints)
  {
    Clipper2Lib::Path64 polygon;
    Clipper2Lib::details::MakePathGeneric(p_points, numPoints * 2, polygon);
    g_polygons.emplace_back(std::move(polygon));
  }

  unsigned int __cdecl CU_UnitePolygons()
  {
    const auto unitedPolygons = Clipper2Lib::Union(g_polygons, Clipper2Lib::FillRule::NonZero);
    g_polygons.clear();
    g_contours.reserve(unitedPolygons.size() * 2);

    for (const auto& polygon : unitedPolygons)
    {
      std::vector<float> contour;
      contour.reserve(polygon.size() * 2);
      for (const auto& point : polygon)
      {
        contour.push_back(static_cast<float>(point.x));
        contour.push_back(static_cast<float>(point.y));
      }
      g_contours.emplace_back(std::move(contour));
    }
    return static_cast<unsigned int>(g_contours.size());
  }

  unsigned int __cdecl CU_GetContour(unsigned int contourIndex, float** p_points)
  {
    (*p_points) = g_contours.at(contourIndex).data();
    return static_cast<unsigned int>(g_contours.at(contourIndex).size() / 2);
  }
}
