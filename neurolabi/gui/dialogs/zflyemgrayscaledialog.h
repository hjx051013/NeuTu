#ifndef ZFLYEMGRAYSCALEDIALOG_H
#define ZFLYEMGRAYSCALEDIALOG_H

#include <QDialog>

class ZProofreadWindow;
class ZFlyEmProofMvc;
class ZIntPoint;
class ZIntCuboid;
class QRect;
class ZStackViewParam;

namespace Ui {
class ZFlyEmGrayscaleDialog;
}

class ZFlyEmGrayscaleDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ZFlyEmGrayscaleDialog(QWidget *parent = 0);
  ~ZFlyEmGrayscaleDialog();

  ZProofreadWindow* getMainWindow() const;
  ZFlyEmProofMvc* getFlyEmProofMvc() const;

  ZIntPoint getOffset() const;
  ZIntPoint getSize() const;
  ZIntCuboid getBoundBox() const;
  ZIntPoint getFirstCorner() const;
  ZIntPoint getLastCorner() const;

  int getOffsetX() const;
  int getOffsetY() const;
  int getOffsetZ() const;
  int getWidth() const;
  int getHeight() const;
  int getDepth() const;

  void setOffset(int x, int y, int z);
  void setWidth(int width);
  void setHeight(int height);
  void setDepth(int depth);

  bool isFullRange() const;
  bool isSparse() const;

private slots:
  void useCurrentOffset();
  void useViewCenter();
  void useViewPort();

private:
  void connectSignalSlot();
  ZStackViewParam getViewParam() const;

private:
  Ui::ZFlyEmGrayscaleDialog *ui;

};

#endif // ZFLYEMGRAYSCALEDIALOG_H
