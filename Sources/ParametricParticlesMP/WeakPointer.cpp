#include "StdH.h"
#include "WeakPointer.h"

#define ID_WEAK_PTRS "WKPT"

WeakPointer::WeakPointer()
: mp_entity(NULL)
{
}

WeakPointer::~WeakPointer()
{
  if (mp_entity)
    static_cast<CRationalEntity_EnableWeakPointer*>(mp_entity)->RemWeakRef(*this);
}

WeakPointer& WeakPointer::operator=(CEntity* entity)
{
  if (entity == mp_entity)
    return *this;

  if (mp_entity)
    static_cast<CRationalEntity_EnableWeakPointer*>(mp_entity)->RemWeakRef(*this);
  mp_entity = NULL;

  CRationalEntity_EnableWeakPointer* wpr = static_cast<CRationalEntity_EnableWeakPointer*>(entity);
  if (wpr)
  {
    mp_entity = entity;
    wpr->AddWeakRef(*this);
  }

  return *this;
}

CEntity* WeakPointer::Get() const
{
  return mp_entity;
}

void WeakPointer::Write(CTStream* strm) const
{  
  strm->WriteID_t(CChunkID(ID_WEAK_PTRS));
  CEntity* parent = Get();
  if (parent)
    (*strm) << parent->en_ulID;
  else
    (*strm) << ULONG(0);
}

void WeakPointer::Read(CEntity* self, CTStream* strm)
{
  if (strm->PeekID_t() == CChunkID(ID_WEAK_PTRS))
  {
    strm->ExpectID_t(CChunkID(ID_WEAK_PTRS));
    ULONG id = 0;
    (*strm) >> id;
    if (id != 0 && _bWorldEditorApp)
      *this = self->GetWorld()->EntityFromID(id);
  }
}

/*****************************************************************/

CRationalEntity_EnableWeakPointer::CRationalEntity_EnableWeakPointer()
{
  m_references.SetAllocationStep(5);
}

CRationalEntity_EnableWeakPointer::~CRationalEntity_EnableWeakPointer()
{
  while (m_references.Count() > 0)
    RemWeakRef(*m_references[0]);
}

void CRationalEntity_EnableWeakPointer::AddWeakRef(WeakPointer& ptr)
{
  for (INDEX i = 0; i < m_references.Count(); ++i)
    if (m_references[i] == &ptr)
      return;
  m_references.Push() = &ptr;
}

void CRationalEntity_EnableWeakPointer::RemWeakRef(WeakPointer& ptr)
{
  if (m_references.Count() == 0)
    return;

  INDEX pos = -1;
  for (INDEX i = 0; i < m_references.Count(); ++i)
    if (m_references[i] == &ptr)
    {
      pos = i;
      break;
    }

  if (pos == -1)
    return;

  if (pos != m_references.Count()-1)
    m_references[pos] = m_references[m_references.Count()-1];

  m_references.Pop();
  ptr.mp_entity = NULL;
}
