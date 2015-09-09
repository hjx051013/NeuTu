#include "zflyembodyannotationdialog.h"
#include "ui_zflyembodyannotationdialog.h"
#include "zflyembodyannotation.h"
#include "neutube.h"

ZFlyEmBodyAnnotationDialog::ZFlyEmBodyAnnotationDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ZFlyEmBodyAnnotationDialog)
{
  ui->setupUi(this);

  if (NeuTube::GetCurrentUserName() == "takemuras" ||
      NeuTube::GetCurrentUserName() == "zhaot") {
    ui->statusComboBox->addItem("Finalized");
  }
  setNameEdit(ui->nameComboBox->currentText());

//  ui->commentLineEdit->setEnabled(false);

  connectSignalSlot();
}

ZFlyEmBodyAnnotationDialog::~ZFlyEmBodyAnnotationDialog()
{
  delete ui;
}

void ZFlyEmBodyAnnotationDialog::connectSignalSlot()
{
  connect(ui->nameComboBox, SIGNAL(currentIndexChanged(QString)),
          this, SLOT(setNameEdit(QString)));
}

void ZFlyEmBodyAnnotationDialog::setPrevUser(const std::string &name)
{
  if (!name.empty()) {
    ui->userLabel->setText(QString("Previously annotated by %1").arg(name.c_str()));
  } else {
    ui->userLabel->setText("");
  }
}

void ZFlyEmBodyAnnotationDialog::setBodyId(uint64_t bodyId)
{
  m_bodyId = bodyId;
  ui->bodyIdLabel->setText(QString("%1").arg(bodyId));
}

QString ZFlyEmBodyAnnotationDialog::getComment() const
{
  return ui->commentLineEdit->text();
}

void ZFlyEmBodyAnnotationDialog::setNameEdit(const QString &name)
{
  if (name != "---") {
    ui->nameLineEdit->setText(name);
  } else {
    ui->nameLineEdit->clear();
  }
}

QString ZFlyEmBodyAnnotationDialog::getStatus() const
{
  return ui->statusComboBox->currentText();
  /*
  if (ui->skipCheckBox->isChecked()) {
    return "Skip";
  }

  return "";
  */
}

QString ZFlyEmBodyAnnotationDialog::getName() const
{
  return ui->nameLineEdit->text();
}

QString ZFlyEmBodyAnnotationDialog::getType() const
{
  return "";
}

uint64_t ZFlyEmBodyAnnotationDialog::getBodyId() const
{
  return m_bodyId;
}

ZFlyEmBodyAnnotation ZFlyEmBodyAnnotationDialog::getBodyAnnotation() const
{
  ZFlyEmBodyAnnotation annotation;
  annotation.setBodyId(getBodyId());
  annotation.setComment(getComment().toStdString());
  annotation.setStatus(getStatus().toStdString());
  annotation.setName(getName().toStdString());
  annotation.setType(getType().toStdString());
  annotation.setUser(NeuTube::GetCurrentUserName());

  return annotation;
}

void ZFlyEmBodyAnnotationDialog::setComment(const std::string &comment)
{
  ui->commentLineEdit->setText(comment.c_str());
//  ui->orphanCheckBox->setChecked(comment == "Orphan");
}

void ZFlyEmBodyAnnotationDialog::setStatus(const std::string &status)
{
  int index = ui->statusComboBox->findText(status.c_str(), Qt::MatchExactly);
  if (index >= 0) {
    ui->statusComboBox->setCurrentIndex(index);
  } else {
    ui->statusComboBox->setCurrentIndex(0);
  }
//  ui->skipCheckBox->setChecked(status == "Skip");
}

void ZFlyEmBodyAnnotationDialog::setName(const std::string &name)
{
  ui->nameLineEdit->setText(name.c_str());
}

void ZFlyEmBodyAnnotationDialog::setType(const std::string &/*type*/)
{
}

void ZFlyEmBodyAnnotationDialog::loadBodyAnnotation(
    const ZFlyEmBodyAnnotation &annotation)
{
  setBodyId(annotation.getBodyId());
  setPrevUser(annotation.getUser());

  setComment(annotation.getComment());
  setStatus(annotation.getStatus());
  setName(annotation.getName());
  setType(annotation.getType());
}
