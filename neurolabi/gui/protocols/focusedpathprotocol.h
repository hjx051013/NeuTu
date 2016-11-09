#ifndef FOCUSEDPATHPROTOCOL_H
#define FOCUSEDPATHPROTOCOL_H

#include <QtGui>

#include "zjsonobject.h"

#include "protocoldialog.h"

namespace Ui {
class FocusedPathProtocol;
}

class FocusedPathProtocol : public ProtocolDialog
{
    Q_OBJECT

public:
    explicit FocusedPathProtocol(QWidget *parent = 0, std::string variation = VARIATION_BODY);
    ~FocusedPathProtocol();
    bool initialize();
    static const std::string VARIATION_BODY;
    static const std::string VARIATION_BOOKMARK;

signals:
    void protocolCompleting();
    void protocolExiting();
    void requestSaveProtocol(ZJsonObject data);
    void bodyListLoaded();

public slots:
    void loadDataRequested(ZJsonObject data);

private slots:
    void onExitButton();
    void onCompleteButton();
    void onBodyListsLoaded();

private:
    static const std::string KEY_VERSION;
    static const int m_fileVersion;
    static const std::string KEY_VARIATION;
    static const std::string KEY_BODYID;

    Ui::FocusedPathProtocol *ui;
    std::string m_variation;
    QList<uint64_t> m_bodies;
    QStandardItemModel * m_edgeModel;
    void saveState();
    void variationError(std::string variation);
    void loadBodiesFromBookmarks();
};

#endif // FOCUSEDPATHPROTOCOL_H
