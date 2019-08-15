#ifndef COMPRESSSETTING_H
#define COMPRESSSETTING_H

#include <QWidget>
#include <DSuggestButton>
#include <QImage>
#include <QComboBox>
#include <dlabel.h>
#include <DLineEdit>
#include "utils.h"
#include <dimagebutton.h>
#include <dswitchbutton.h>
#include <dpasswordedit.h>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

class CompressSetting :public QWidget
{
public:
    CompressSetting(QWidget* parent = 0);
    ~CompressSetting();

    void InitUI();
    void InitConnection();

private:
    DSuggestButton* m_nextbutton;
    QPixmap m_compressicon;
    QComboBox* m_compresstype;
    DLineEdit* m_filename;
    DLineEdit* m_savepath;
    DLabel* m_pixmaplabel;
    DSuggestButton* m_pathbutton;
    QVBoxLayout *m_fileLayout;

    QHBoxLayout *m_moresetlayout;
    DSwitchButton* m_moresetbutton;
    DPasswordEdit* m_password;
    QHBoxLayout *m_file_secretlayout;
    DSwitchButton* m_file_secret;
    QHBoxLayout *m_splitlayout;
    DLineEdit* m_splitnumedit;
    DSuggestButton* m_plusbutton;
    DSuggestButton* m_minusbutton;
    DLabel* m_encryptedlabel;
    DLabel* m_splitcompress;

public slots:
    void onPathButoonClicked();
    void onNextButoonClicked();
    void onAdvanceButtonClicked(bool status);
};

#endif // COMPRESSSETTING_H
