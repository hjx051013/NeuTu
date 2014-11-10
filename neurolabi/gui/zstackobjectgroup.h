#ifndef ZSTACKOBJECTGROUP_H
#define ZSTACKOBJECTGROUP_H

#include <QList>
#include <QSet>
#include <QMap>
#include <set>

#include "zstackobject.h"

/*!
 * \brief The aggregate class of ZStackObject
 *
 * No NULL object is allowed to be included in the group.
 */
typedef QSet<ZStackObject*> TStackObjectSet;
typedef QList<ZStackObject*> TStackObjectList;
typedef bool (*TObjectTest)(const ZStackObject*);

class ZStackObjectGroup : public QList<ZStackObject*>
{
public:
  ZStackObjectGroup();
  typedef QMap<ZStackObject::EType, TStackObjectList> TObjectListMap;
  typedef QMap<ZStackObject::EType, TStackObjectSet> TObjectSetMap;

  /*!
   * \brief Get the max Z order
   *
   * \return 0 if the group is empty
   */
  int getMaxZOrder() const;

  void setSelected(ZStackObject *obj, bool selected);
  void setSelected(bool selected);
  void setSelected(ZStackObject::EType type, bool selected);

  const ZStackObject *getLastObject(ZStackObject::EType type) const;

  ZStackObject* findFirstSameSource(const ZStackObject *obj) const;
  ZStackObject* findFirstSameSource(
      ZStackObject::EType type, const std::string &source) const;
  TStackObjectList findSameSource(const ZStackObject *obj) const;
  TStackObjectList findSameSource(
      ZStackObject::EType type, const std::string &source) const;


  template<typename InputIterator>
  QList<ZStackObject*> findSameSource(
      const InputIterator begin, const InputIterator end) const;

  void add(const ZStackObject *obj, bool uniqueSource);
  QList<ZStackObject*> addU(const ZStackObject *obj);

  template <typename InputIterator>
  void add(const InputIterator begin, const InputIterator end,
           bool uniqueSource);

  void addInFront(ZStackObject *obj, bool uniqueSource);

  /*!
   * \brief Take an object
   *
   * \a obj is removed from the group.
   *
   * \return \a obj if it is in the group. Otherwise it returns NULL.
   */
  ZStackObject* take(ZStackObject *obj);
  TStackObjectList take(TObjectTest testFunc);
  TStackObjectList take(ZStackObject::EType type, TObjectTest testFunc);
  TStackObjectList take(ZStackObject::EType type);
  TStackObjectList takeSelected();
  TStackObjectList takeSelected(ZStackObject::EType type);
  TStackObjectList takeSameSource(
      ZStackObject::EType type, const std::string &source);


  /*!
   * \brief Remove an object
   *
   * Nothing will be done if \a obj is not in the group.
   *
   * \param isDeleting Delete the object or not.
   * \return true iff \a obj is removed.
   */
  bool removeObject(ZStackObject *obj, bool isDeleting = true);
  bool removeObject(ZStackObject::EType type, bool deleting = true);
  bool removeObject(const TStackObjectSet &objSet, bool deleting = true);

  bool removeSelected(bool deleting = true);
  bool removeSelected(ZStackObject::EType type, bool deleting = true);

  template <typename InputIterator>
  void removeObject(InputIterator begin, InputIterator end,
                    bool deleting = true);

  /*!
   * \brief Remove all objects
   */
  void removeAllObject(bool deleting = true);

  TStackObjectList& getObjectList(ZStackObject::EType type);
  const TStackObjectList& getObjectList(ZStackObject::EType type) const;
  TStackObjectList getObjectList(ZStackObject::EType type,
                                 TObjectTest testFunc) const;

  TStackObjectSet& getSelectedSet(ZStackObject::EType type);
  const TStackObjectSet& getSelectedSet(ZStackObject::EType type) const;

  TStackObjectSet getObjectSet(ZStackObject::EType type) const;

  bool hasObject(ZStackObject::EType type) const;
  bool hasSelected() const;
  bool hasSelected(ZStackObject::EType type) const;

private:
  bool remove_p(TStackObjectSet &objSet, ZStackObject *obj);

private:
  TObjectListMap m_sortedGroup;
  TObjectSetMap m_selectedSet;
};

template <typename InputIterator>
void ZStackObjectGroup::add(
    const InputIterator begin, const InputIterator end, bool uniqueSource)
{
  for (InputIterator iter = begin; iter != end; ++iter) {
    add(*iter, uniqueSource);
  }
}

template <typename InputIterator>
void ZStackObjectGroup::removeObject(
    InputIterator begin, InputIterator end, bool deleting)
{
  TStackObjectSet objSet;

  for (InputIterator iter = begin; iter != end; ++iter) {
    objSet.insert(*iter);
  }

  removeObject(objSet, deleting);
}

template<typename InputIterator>
TStackObjectList ZStackObjectGroup::findSameSource(
    const InputIterator begin, const InputIterator end) const
{
  TStackObjectList objList;
  for (InputIterator iter = begin; iter != end; ++iter) {
    objList.append(findSameSource(*iter));
  }

  return objList;
}
#endif // ZSTACKOBJECTGROUP_H
