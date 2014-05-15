#include "ztilegraphicsitem.h"

bool ZTileGraphicsItem::loadJsonObject(const ZJsonObject &obj)
{
  if (m_tileInfo.loadJsonObject(obj)) {
    setOffset(m_tileInfo.getOffset().x(), m_tileInfo.getOffset().y());
    setPixmap(QPixmap(QString::fromStdString((m_tileInfo.getImageSource()))));
    setToolTip(m_tileInfo.getSource().c_str());

    return true;
  }

  return false;
}
