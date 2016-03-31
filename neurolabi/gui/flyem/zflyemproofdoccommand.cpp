#include "zflyemproofdoccommand.h"


#include "flyem/zflyemproofdoc.h"

//////////////////////////////////////////
ZFlyEmProofDocCommand::MergeBody::MergeBody(
    ZStackDoc *doc, QUndoCommand *parent)
  : ZUndoCommand(parent), m_doc(doc)
{

}

ZFlyEmProofDoc* ZFlyEmProofDocCommand::MergeBody::getCompleteDocument()
{
  return qobject_cast<ZFlyEmProofDoc*>(m_doc);
}

void ZFlyEmProofDocCommand::MergeBody::redo()
{
  getCompleteDocument()->getBodyMerger()->redo();
  getCompleteDocument()->updateBodyObject();

  getCompleteDocument()->notifyBodyMerged();
  getCompleteDocument()->notifyBodyMergeEdited();
//  m_doc->notifyObject3dScanModified();
}

void ZFlyEmProofDocCommand::MergeBody::undo()
{
  ZFlyEmBodyMerger::TLabelMap mapped =
      getCompleteDocument()->getBodyMerger()->undo();

  std::set<uint64_t> bodySet;
  for (ZFlyEmBodyMerger::TLabelMap::const_iterator iter = mapped.begin();
       iter != mapped.end(); ++iter) {
    bodySet.insert(iter.key());
    bodySet.insert(iter.value());
  }

  for (std::set<uint64_t>::const_iterator iter = bodySet.begin();
       iter != bodySet.end(); ++iter) {
    uint64_t bodyId = *iter;
    if (!getCompleteDocument()->getBodyMerger()->isMerged(bodyId)) {
      getCompleteDocument()->notifyBodyIsolated(bodyId);
    }
  }

  getCompleteDocument()->updateBodyObject();

  getCompleteDocument()->notifyBodyUnmerged();
  getCompleteDocument()->notifyBodyMergeEdited();
//  m_doc->notifyObject3dScanModified();
}
