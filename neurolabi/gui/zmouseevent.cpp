#include "zmouseevent.h"
#include <QMouseEvent>

ZMouseEvent::ZMouseEvent() : m_buttons(Qt::NoButton),
  m_action(ZMouseEvent::ACTION_NONE), m_modifiers(Qt::NoModifier),
  m_isInStack(false)
{
  m_sliceAxis = neutube::EAxis::Z;
}

ZPoint ZMouseEvent::getPosition(neutube::ECoordinateSystem cs) const
{
  switch (cs) {
  case neutube::ECoordinateSystem::WIDGET:
    return m_widgetPosition.toPoint();
  case neutube::ECoordinateSystem::STACK:
    return m_stackPosition;
  case neutube::ECoordinateSystem::RAW_STACK:
    return m_rawStackPosition;
  case neutube::ECoordinateSystem::SCREEN:
    return m_globalPosition.toPoint();
  case neutube::ECoordinateSystem::ORGDATA:
    return m_dataPosition;
  default:
    break;
  }

  return ZPoint(0, 0, 0);
}

void ZMouseEvent::setRawStackPosition(const ZPoint &pt)
{
  setRawStackPosition(pt.x(), pt.y(), pt.z());
}

void ZMouseEvent::setRawStackPosition(double x, double y, double z)
{
  m_rawStackPosition.set(x, y, z);
}

void ZMouseEvent::setStackPosition(const ZPoint &pt)
{
  m_stackPosition = pt;
}

void ZMouseEvent::setDataPositoin(const ZPoint &pt)
{
  m_dataPosition = pt;
}

void ZMouseEvent::set(QMouseEvent *event, int z)
{
  //m_button = event->button();
  m_buttons = event->buttons();
  m_action = ACTION_NONE;
  m_modifiers = event->modifiers();
  m_widgetPosition.set(event->x(), event->y(), z);
  m_globalPosition.set(event->globalX(), event->globalY(), z);
}

void ZMouseEvent::set(QMouseEvent *event, EAction action, int z)
{
  set(event, z);
  m_action = action;
  if (action == ACTION_RELEASE) {
    m_buttons = event->button();
  }
}

neutube::EAxis ZMouseEvent::getSliceAxis() const
{
  return m_sliceAxis;
}

void ZMouseEvent::setSliceAxis(neutube::EAxis axis)
{
  m_sliceAxis = axis;
}

void ZMouseEvent::setPressEvent(QMouseEvent *event, int z)
{
  set(event, ACTION_PRESS, z);
}

void ZMouseEvent::setMoveEvent(QMouseEvent *event, int z)
{
  set(event, ACTION_MOVE, z);
}

void ZMouseEvent::setReleaseEvent(QMouseEvent *event, int z)
{
  set(event, ACTION_RELEASE, z);
}

bool ZMouseEvent::isNull() const
{
  return m_action == ACTION_NONE;
}

void ZMouseEvent::print() const
{
  std::cout << "Mouse event: ";
  if (getButtons() & Qt::LeftButton) {
    std::cout << "Left button; ";
  }
  if (getButtons() & Qt::RightButton) {
    std::cout << "Right button; ";
  }
  if (getButtons() & Qt::MidButton) {
    std::cout << "Middle button; ";
  }

  switch (getAction()) {
  case ACTION_DOUBLE_CLICK:
    std::cout << "Double click; ";
    break;
  case ACTION_MOVE:
    std::cout << "Move; ";
    break;
  case ACTION_PRESS:
    std::cout << "Press; ";
    break;
  case ACTION_RELEASE:
    std::cout << "Release; ";
    break;
  default:
    break;
  }

  if (getModifiers() & Qt::ShiftModifier) {
    std::cout << "Shift; ";
  }
  if (getModifiers() & Qt::ControlModifier) {
    std::cout << "Control; ";
  }
  if (getModifiers() & Qt::AltModifier) {
    std::cout << "Alt; ";
  }

  std::cout << std::endl;
}
