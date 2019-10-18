#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <DTitlebar>
#include <DDialog>
#include <DPushButton>
#include <DLabel>
#include <DProgressBar>
#include "extractpausedialog.h"
#include <DApplicationHelper>

DWIDGET_USE_NAMESPACE

class ProgressDialog: public DDialog
{
    Q_OBJECT
public:
    explicit ProgressDialog(QWidget *parent = 0);
    void initUI();
    void initConnect();

    void setCurrentTask(const QString &file);
    void setCurrentFile(const QString &file);
    void setProcess(unsigned long  value);
    void setFinished(const QString &path);
    void showdialog();
    bool isshown();
    void clearprocess();

    void closeEvent(QCloseEvent *) override;

signals:
    void stopExtract();


public slots:
    void slotextractpress(int index);

private:
    int m_defaultWidth = 380;
    int m_defaultHeight = 120;

    DLabel* m_tasklable;
    DLabel* m_filelable;
    DProgressBar* m_circleprogress;

    ExtractPauseDialog* m_extractdialog;

};

#endif // PROGRESSDIALOG_H
