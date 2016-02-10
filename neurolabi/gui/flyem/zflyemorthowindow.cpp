#include "zflyemorthowindow.h"

#include "flyem/zflyemorthowidget.h"
#include "flyem/zflyemorthodoc.h"

ZFlyEmOrthoWindow::ZFlyEmOrthoWindow(const ZDvidTarget &target, QWidget *parent) :
  QMainWindow(parent)
{
  m_orthoWidget = new ZFlyEmOrthoWidget(target, this);
  setCentralWidget(m_orthoWidget);
  connect(m_orthoWidget, SIGNAL(bookmarkEdited(int,int,int)),
          this, SIGNAL(bookmarkEdited(int,int,int)));
  connect(m_orthoWidget, SIGNAL(synapseEdited(int,int,int)),
          this, SIGNAL(synapseEdited(int,int,int)));
  connect(m_orthoWidget, SIGNAL(zoomingTo(int,int,int)),
          this, SIGNAL(zoomingTo(int,int,int)));
}


void ZFlyEmOrthoWindow::updateData(const ZIntPoint &center)
{
  m_orthoWidget->moveTo(center);
}

void ZFlyEmOrthoWindow::downloadBookmark(int x, int y, int z)
{
  getDocument()->downloadBookmark(x, y, z);
}

ZFlyEmOrthoDoc *ZFlyEmOrthoWindow::getDocument() const
{
  return m_orthoWidget->getDocument();
}
void ZFlyEmOrthoWindow::copyBookmarkFrom(ZFlyEmProofDoc *doc)
{
  getDocument()->copyBookmarkFrom(doc);
}
