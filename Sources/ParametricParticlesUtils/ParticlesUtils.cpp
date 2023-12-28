#include <Engine/Engine.h>

#include "API.h"
#include "ParticlesUtils.h"

FLOAT GetDiscreteGraphValueSince(const CStaticArray<FLOAT>& graph, FLOAT birthTime, BOOL loop)
{
  if (graph.Count() == 0)
    return 1.0f;

  INDEX graphIndex = ClampDn(INDEX(floor(0.5f + (_pTimer->CurrentTick() - birthTime) / CTimer::TickQuantum)), INDEX(0));
  if (loop)
    graphIndex %= graph.Count();
  else
    graphIndex = ClampUp(graphIndex, INDEX(graph.Count() - 1));

  return graph[graphIndex];
}

FLOAT GetGraphValueAt(const CStaticArray<FLOAT2D>& graph, FLOAT f)
{
  if (graph.Count() == 0)
    return 1.0f;

  INDEX i = LowerBound(graph, f);
  if (i >= graph.Count()) {
    return graph[graph.Count()-1](2);
  }
  if (i == 0) {
    return graph[0](2);
  }
  const FLOAT2D& a = graph[i-1];
  const FLOAT2D& b = graph[i];
  const FLOAT af = a(1);
  const FLOAT bf = b(1);
  const FLOAT pf = (f - af) / (bf - af);
  return Lerp(a(2), b(2), pf);
}

void RecacheGraph(CStaticArray<FLOAT2D>& cache, const CTString& graph)
{
  cache.Clear();
  INDEX numCoords = 0;
  if (graph.Length() <= 0) {
    return;
  }
  INDEX offset = 0;
  INDEX pos = 0;
  FLOAT dummy;
  while (sscanf(graph.str_String + offset, "%f %f%n", &dummy, &dummy, &pos) == 2) {
    offset += pos;
    ++numCoords;
  }

  if (numCoords == 0) {
    return;
  }
  offset = 0;
  pos = 0;
  cache.New(numCoords);
  for (INDEX i = 0; i < numCoords; ++i) {
    sscanf(graph.str_String + offset, "%f %f%n", &cache[i](1), &cache[i](2), &pos);
    offset += pos;
  }
}

void RecacheGraphDiscrete(CStaticArray<FLOAT>& cache, const CTString& graphStr)
{
  cache.Clear();
  CStaticArray<FLOAT2D> graph;
  RecacheGraph(graph, graphStr);
  if (graph.Count() == 0)
    return;

  const FLOAT maxTime = graph[graph.Count() - 1](1);
  if (maxTime <= CTimer::TickQuantum)
  {
    cache.New(1);
    cache[0] = graph[graph.Count() - 1](2);
    return;
  }

  cache.New(floor(maxTime / CTimer::TickQuantum + 0.5f) + 1);
  for (INDEX i = 0; i < cache.Count(); ++i)
  {
    const TIME currTime = (i * CTimer::TickQuantum);
    cache[i] = GetGraphValueAt(graph, currTime);
  }
}

INDEX LowerBound(const CStaticArray<FLOAT2D>& arr, const FLOAT value)
{
  INDEX count = arr.Count();
  INDEX step = 0;
  INDEX first = 0;
  INDEX it = 0;

  while (count > 0)
  {
    it = first; 
    step = count / 2; 
    it += step;

    if (arr[it](1) < value)
    {
      first = ++it; 
      count -= step + 1; 
    }
    else
    {
      count = step;
    }
  }

  return first;
}

void EditGraphVariable(const CEntity* self, BOOL& property, CTString& graph, ULONG flags)
{
  if (!property)
    return;
  property = FALSE;

  const CTString graphExe = _fnmApplicationPath + "Bin\\Graph.exe";
  const CTString tmpGraphFile = "Temp\\graph.txt";
  CTString propertyName = "";

  const SLONG offset = ((const char*)&property) - ((const char*)self);
  const CDLLEntityClass* dllClass = self->en_pecClass->ec_pdecDLLClass;
  for (INDEX i = 0; i < dllClass->dec_ctProperties; ++i)
  {
    const CEntityProperty& prop = dllClass->dec_aepProperties[i];
    if (prop.ep_slOffset == offset)
    {
      propertyName = prop.ep_strName;
      break;
    }
  }

  FILE* graphFile = fopen((_fnmApplicationPath + tmpGraphFile).str_String, "w");
  if (!graphFile)
  {
    WarningMessage("Failed to write into temp file %s!", tmpGraphFile.str_String);
    return;
  }
  fprintf(graphFile, "%d\n", flags);
  fprintf(graphFile, "%s\n", propertyName.str_String);
  fprintf(graphFile, graph.str_String);
  fclose(graphFile);

  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  if (!CreateProcess(graphExe.str_String, tmpGraphFile.str_String, NULL, NULL, FALSE, 0, NULL, _fnmApplicationPath.str_String, &si, &pi))
  {
    WarningMessage("Failed to launch Graph.exe! Repair your installation.");
    return;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);

  DWORD exitCode = 0;
  if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode == 0)
  {
    graphFile = fopen((_fnmApplicationPath + tmpGraphFile).str_String, "r");
    if (!graphFile)
    {
      WarningMessage("Failed to read from temp file %s!", tmpGraphFile.str_String);
      return;
    }
    graph = "";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), graphFile))
      graph += buffer;
    fclose(graphFile);
  }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
}

#ifdef USE_CUSTOM_PARTICLE_PROJECTION
CProjection3D* Particle_GetProjection()
{
  static const int enginedllBaseAddress = reinterpret_cast<int>(&CEntity::HandleSentEvents) - 0xFEC80;
  static CProjection3D** ppProjection = reinterpret_cast<CProjection3D**>(enginedllBaseAddress + 0x1F64C8);
  return *ppProjection;
}
#endif
