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
  friend class WeakPointerReferable;

  CEntity* mp_entity;
};

class WeakPointerReferable
{
public:
  WeakPointerReferable();
  ~WeakPointerReferable();

private:
  friend class WeakPointer;

  void AddWeakRef(WeakPointer& ptr);
  void RemWeakRef(WeakPointer& ptr);

protected:
  CStaticStackArray<WeakPointer*> m_references;
};

template<typename TBase>
class EnableWeakPointer : public TBase, public WeakPointerReferable
{
};

#endif // WEAK_POINTER_42_H
