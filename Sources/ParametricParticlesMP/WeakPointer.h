#ifndef WEAK_POINTER_42_H
#define WEAK_POINTER_42_H

class WeakPointer
{
public:
  WeakPointer();
  ~WeakPointer();
  WeakPointer& operator=(CEntity* entity);
  CEntity* Get() const;
  void Write(CTStream* strm) const;
  void Read(CEntity* self, CTStream* strm);

private:
  friend class CRationalEntity_EnableWeakPointer;

  CEntity* mp_entity;
};

class CRationalEntity_EnableWeakPointer : public CRationalEntity
{
public:
  CRationalEntity_EnableWeakPointer();
  ~CRationalEntity_EnableWeakPointer();

private:
  friend class WeakPointer;

  void AddWeakRef(WeakPointer& ptr);
  void RemWeakRef(WeakPointer& ptr);

protected:
  CStaticStackArray<WeakPointer*> m_references;
};

#define CRationalEntity_EnableWeakPointer_DLLClass CRationalEntity_DLLClass

#endif // WEAK_POINTER_42_H
