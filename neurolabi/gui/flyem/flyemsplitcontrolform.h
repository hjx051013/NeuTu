#ifndef FLYEMSPLITCONTROLFORM_H
#define FLYEMSPLITCONTROLFORM_H

#include <QWidget>
//#include "zflyembodysplitproject.h"

#include "flyem/zflyembookmarklistmodel.h"

class QMenu;
class ZFlyEmBodySplitProject;

namespace Ui {
class FlyEmSplitControlForm;
}

class FlyEmSplitControlForm : public QWidget
{
  Q_OBJECT

public:
  explicit FlyEmSplitControlForm(QWidget *parent = 0);
  ~FlyEmSplitControlForm();

signals:
  void exitingSplit();
  void quickViewTriggered();
  void splitQuickViewTriggered();
  void bodyViewTriggered();
  void splitViewTriggered();
  void changingSplit(uint64_t);
  void savingSeed();
  void committingResult();
  void zoomingTo(int x, int y, int z);
  void loadingBookmark(QString);
  void recoveringSeed();
  void exportingSeed();
  void importingSeed();
  void selectingSeed();
  void selectingAllSeed();
  void loadingSynapse();
  void bookmarkChecked(QString key, bool);
  void bookmarkChecked(ZFlyEmBookmark*);

public slots:
  void updateBodyWidget(uint64_t bodyId);
  void goToPosition();
  void recoverSeed();
  void exportSeed();
  void importSeed();
  void selectSeed();
  void selectAllSeed();

private slots:
  void slotTest();
  void setSplit(uint64_t bodyId);
  void changeSplit();
  void commitResult();
  void updateBookmarkTable(ZFlyEmBodySplitProject *project);
  void clearBookmarkTable(ZFlyEmBodySplitProject *project);
  void locateBookmark(const QModelIndex &index);
  void loadBookmark();
  void checkCurrentBookmark();
  void uncheckCurrentBookmark();
  void checkCurrentBookmark(bool checking);

private:
  void setupWidgetBehavior();
  void createMenu();

private:
  Ui::FlyEmSplitControlForm *ui;
  ZFlyEmBookmarkListModel m_bookmarkList;
  //ZFlyEmBodySplitProject m_project;
  QMenu *m_mainMenu;
//  QMenu *m_bookmarkContextMenu;
};

#endif // FLYEMSPLITCONTROLFORM_H
