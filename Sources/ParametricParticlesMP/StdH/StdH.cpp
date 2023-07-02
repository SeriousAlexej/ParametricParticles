#include "StdH.h"

bool InWED()
{
  if (!_bWorldEditorApp)
    return false;

  for (INDEX i = 0; i < _pNetwork->ga_aplsPlayers.Count(); ++i)
    if (_pNetwork->ga_aplsPlayers[i].IsActive())
      return false;

  return true;
}