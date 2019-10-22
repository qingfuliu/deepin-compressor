/*
 * Copyright (C) 2017 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     rekols <rekols@foxmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "utils.h"


#include <QSvgWidget>
#include <QDebug>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <DDesktopServices>
#include <QMimeDatabase>
#include <QFileIconProvider>
#include "pluginmanager.h"

//#include "archivejob.h"
#include "jobs.h"



MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent),
      m_mainWidget(new QWidget),
      m_mainLayout(new QStackedLayout(m_mainWidget)),
      m_homePage(new HomePage),
      m_UnCompressPage(new UnCompressPage),
      m_CompressPage(new CompressPage),
      m_CompressSetting(new CompressSetting),
      m_Progess(new Progress),
      m_CompressSuccess(new Compressor_Success),
      m_CompressFail(new Compressor_Fail),
      m_encryptionpage(new EncryptionPage),
      m_progressdialog(new ProgressDialog),
      m_settingsDialog(new SettingDialog),
      m_encodingpage(new EncodingPage),
      m_settings(new QSettings(QDir(Utils::getConfigPath()).filePath("config.conf"),
                               QSettings::IniFormat))
{
    m_encryptionjob = nullptr;
    m_encryptiontype = Encryption_NULL;
    m_isrightmenu = false;
    m_model = new ArchiveModel(this);
    m_filterModel = new ArchiveSortFilterModel(this);
    InitUI();
    InitConnection();
}

MainWindow::~MainWindow()
{
}

void MainWindow::InitUI()
{
    if (m_settings->value("dir").toString().isEmpty()) {
        m_settings->setValue("dir", "");
    }

    // add widget to main layout.
    m_mainLayout->addWidget(m_homePage);
    m_mainLayout->addWidget(m_UnCompressPage);
    m_mainLayout->addWidget(m_CompressPage);
    m_mainLayout->addWidget(m_CompressSetting);
    m_mainLayout->addWidget(m_Progess);
    m_mainLayout->addWidget(m_CompressSuccess);
    m_mainLayout->addWidget(m_CompressFail);
    m_mainLayout->addWidget(m_encryptionpage);
    m_mainLayout->addWidget(m_encodingpage);
    m_homePage->setAutoFillBackground(true);
    m_UnCompressPage->setAutoFillBackground(true);
    m_CompressPage->setAutoFillBackground(true);
    m_CompressSetting->setAutoFillBackground(true);
    m_Progess->setAutoFillBackground(true);
    m_CompressSuccess->setAutoFillBackground(true);
    m_CompressFail->setAutoFillBackground(true);
    m_encryptionpage->setAutoFillBackground(true);
    m_encodingpage->setAutoFillBackground(true);

    // init window flags.
    setWindowTitle(tr("解压缩"));
    setWindowFlags(windowFlags()&~Qt::WindowMaximizeButtonHint);
    setCentralWidget(m_mainWidget);
    setAcceptDrops(true);

    initTitleBar();

}

void MainWindow::InitConnection()
{
    // connect the signals to the slot function.
    connect(m_homePage, &HomePage::fileSelected, this, &MainWindow::onSelected);
    connect(m_CompressPage, &CompressPage::sigselectedFiles, this, &MainWindow::onSelected);
    connect(m_CompressPage, &CompressPage::sigNextPress, this, &MainWindow::onCompressNext);
    connect(this, &MainWindow::sigZipAddFile, m_CompressPage, &CompressPage::onAddfileSlot);
    connect(this, &MainWindow::sigZipReturn, m_CompressSetting, &CompressSetting::onRetrunPressed);
    connect(m_CompressSetting, &CompressSetting::sigCompressPressed, this, &MainWindow::onCompressPressed);
    connect(m_Progess, &Progress::sigCancelPressed, this, &MainWindow::onCancelCompressPressed);
    connect(m_CompressSuccess, &Compressor_Success::sigQuitApp, this, &MainWindow::onCancelCompressPressed);
    connect(m_titlebutton, &DPushButton::clicked, this, &MainWindow::onTitleButtonPressed);
    connect(this, &MainWindow::sigZipSelectedFiles, m_CompressPage, &CompressPage::onSelectedFilesSlot);
    connect(m_model, &ArchiveModel::loadingFinished,this, &MainWindow::slotLoadingFinished);
    connect(m_UnCompressPage, &UnCompressPage::sigDecompressPress,this, &MainWindow::slotextractSelectedFilesTo);
    connect(m_encryptionpage, &EncryptionPage::sigExtractPassword,this, &MainWindow::SlotExtractPassword);
    connect(m_UnCompressPage, &UnCompressPage::sigextractfiles,this, &MainWindow::slotExtractSimpleFiles);
    connect(m_progressdialog, &ProgressDialog::stopExtract,this, &MainWindow::slotKillExtractJob);
    connect(m_CompressFail, &Compressor_Fail::sigFailRetry,this, &MainWindow::slotFailRetry);
}

void MainWindow::customMessageHandler(const QString &msg)
{
    QString txt;
    txt = msg;

    QFile outFile("/home/deepin/debug.log");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
}

QMenu* MainWindow::createSettingsMenu()
{
    QMenu *menu = new QMenu;

    QAction *openAction = menu->addAction(tr("打开"));
    connect(openAction, &QAction::triggered, this, [this] {
        DFileDialog dialog;
        dialog.setAcceptMode(DFileDialog::AcceptOpen);
        dialog.setFileMode(DFileDialog::ExistingFiles);

        QString historyDir = m_settings->value("dir").toString();
        if (historyDir.isEmpty()) {
            historyDir = QDir::homePath();
        }
        dialog.setDirectory(historyDir);

        const int mode = dialog.exec();

        // save the directory string to config file.
        m_settings->setValue("dir", dialog.directoryUrl().toLocalFile());

        // if click cancel button or close button.
        if (mode != QDialog::Accepted) {
            return;
        }

        onSelected(dialog.selectedFiles());
    });

    QAction *settingsAction = menu->addAction(tr("设置"));
    connect(settingsAction, &QAction::triggered, this, [this] {
        m_settingsDialog->exec();
    });



    return menu;
}


void MainWindow::initTitleBar()
{
    titlebar()->setMenu(createSettingsMenu());
    titlebar()->setFixedHeight(50);

    QIcon icon = QIcon::fromTheme("deepin-compressor");
    m_logo = new DLabel("");
    m_logo->setPixmap(icon.pixmap(QSize(30, 30)));


    m_titlebutton = new DIconButton(DStyle::StandardPixmap::SP_IncreaseElement, this);
    m_titlebutton->setFixedSize(36, 36);
    m_titlebutton->setVisible(false);

    m_titleFrame = new QFrame;
    m_titleFrame->setObjectName("TitleBar");
    QHBoxLayout *leftLayout = new QHBoxLayout;
    leftLayout->addSpacing(6);
    leftLayout->addWidget(m_logo);
    leftLayout->addSpacing(6);
    leftLayout->addWidget(m_titlebutton);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *left_frame = new QFrame();
    left_frame->setFixedWidth(10+6+36+30);
    left_frame->setContentsMargins(0, 0, 0, 0);
    left_frame->setLayout(leftLayout);

    m_titlelabel = new DLabel("");
    m_titlelabel->setMinimumSize(200, TITLE_FIXED_HEIGHT);
    m_titlelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *titlemainLayout = new QHBoxLayout;
    titlemainLayout->setContentsMargins(0, 0, 0, 0);
    titlemainLayout->addWidget(left_frame);
    titlemainLayout->addSpacing(50);
    titlemainLayout->addWidget(m_titlelabel, 0, Qt::AlignCenter);

    m_titleFrame->setLayout(titlemainLayout);
    m_titleFrame->setFixedHeight(TITLE_FIXED_HEIGHT);
    titlebar()->setContentsMargins(0, 0, 0, 0);
    titlebar()->setCustomWidget(m_titleFrame, false);
}

void MainWindow::setQLabelText(QLabel *label, const QString &text)
{
    QFontMetrics cs(label->font());
    int textWidth = cs.width(text);
    if(textWidth > label->width())
    {
        label->setToolTip(text);
    }
    else
    {
         label->setToolTip("");
    }

    QFontMetrics elideFont(label->font());
    label->setText(elideFont.elidedText(text, Qt::ElideMiddle, label->width()));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    const auto *mime = e->mimeData();

    // not has urls.
    if (!mime->hasUrls()) {
        return e->ignore();
    }

    // traverse.
    m_homePage->setIconPixmap(true);
    return e->accept();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *e)
{
    m_homePage->setIconPixmap(false);

    DMainWindow::dragLeaveEvent(e);
}

void MainWindow::dropEvent(QDropEvent *e)
{
    auto *const mime = e->mimeData();

    if (!mime->hasUrls())
        return e->ignore();

    e->accept();

    // find font files.
    QStringList fileList;
    for (const auto &url : mime->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }

        const QString localPath = url.toLocalFile();
        const QFileInfo info(localPath);

        fileList << localPath;
        qDebug()<<fileList;


    }

    m_homePage->setIconPixmap(false);
    onSelected(fileList);
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}



void MainWindow::setEnable()
{
    setAcceptDrops(true);

    // enable titlebar buttons.
    titlebar()->setDisableFlags(Qt::Widget);
}

void MainWindow::setDisable()
{
    setAcceptDrops(false);

    // disable titlebar buttons.
    titlebar()->setDisableFlags(Qt::WindowMinimizeButtonHint
                                | Qt::WindowCloseButtonHint
                                | Qt::WindowMaximizeButtonHint
                                | Qt::WindowSystemMenuHint);
}

void MainWindow::refreshPage()
{
    setAcceptDrops(false);
    m_titlebutton->setVisible(false);
    switch (m_pageid) {
    case PAGE_HOME:
        m_mainLayout->setCurrentIndex(0);
        setQLabelText(m_titlelabel, "");
        setAcceptDrops(true);
        break;
    case PAGE_UNZIP:
        m_mainLayout->setCurrentIndex(1);
//        m_titlelabel->setText(m_decompressfilename);
        setQLabelText(m_titlelabel, m_decompressfilename);
        break;
    case PAGE_ZIP:
        m_mainLayout->setCurrentIndex(2);
        setQLabelText(m_titlelabel, tr("新建归档文件"));
        m_titlebutton->setIcon(DStyle::StandardPixmap::SP_IncreaseElement);
        m_titlebutton->setVisible(true);
        setAcceptDrops(true);
        break;
    case PAGE_ZIPSET:
        m_mainLayout->setCurrentIndex(3);
        setQLabelText(m_titlelabel, tr("新建归档文件"));
        m_titlebutton->setIcon(DStyle::StandardPixmap::SP_ArrowLeave);
        m_titlebutton->setVisible(true);
        break;
    case PAGE_ZIPPROGRESS:
        m_mainLayout->setCurrentIndex(4);
        setQLabelText(m_titlelabel, tr("正在压缩"));
        m_Progess->setFilename(m_decompressfilename);
        break;
    case PAGE_UNZIPPROGRESS:
        m_mainLayout->setCurrentIndex(4);
        setQLabelText(m_titlelabel, tr("正在解压"));
        m_Progess->setFilename(m_decompressfilename);
        break;
    case PAGE_ZIP_SUCCESS:
        m_mainLayout->setCurrentIndex(5);
        setQLabelText(m_titlelabel, "");
        m_CompressSuccess->setstringinfo(tr("压缩成功!"));
        break;
    case PAGE_ZIP_FAIL:
        m_mainLayout->setCurrentIndex(6);
        setQLabelText(m_titlelabel, "");
        m_CompressFail->setFailStr(tr("抱歉，压缩失败！"));
        break;
    case PAGE_UNZIP_SUCCESS:
        m_mainLayout->setCurrentIndex(5);
        setQLabelText(m_titlelabel, "");
        m_CompressSuccess->setCompressPath(m_decompressfilepath);
        m_CompressSuccess->setstringinfo(tr("解压成功！"));
        if(m_settingsDialog->isAutoOpen())
        {
            DDesktopServices::showFolder(QUrl(m_decompressfilepath, QUrl::TolerantMode));
        }
        break;
    case PAGE_UNZIP_FAIL:
        m_mainLayout->setCurrentIndex(6);
        setQLabelText(m_titlelabel, "");
        m_CompressFail->setFailStr(tr("抱歉，解压失败！"));
        break;
    case  PAGE_ENCRYPTION:
        m_mainLayout->setCurrentIndex(7);
        setQLabelText(m_titlelabel, m_decompressfilename);
        break;
    default:
        break;
    }

}

void MainWindow::onSelected(const QStringList &files)
{
    if(files.count() == 1 && Utils::isCompressed_file(files.at(0)))
    {
        if(0 == m_CompressPage->getCompressFilelist().count())
        {
            QFileInfo fileinfo(files.at(0));
            m_decompressfilename = fileinfo.fileName();
            if("" != m_settingsDialog->getCurExtractPath())
            {
               m_UnCompressPage->setdefaultpath(m_settingsDialog->getCurExtractPath());
            }
            else
            {
               m_UnCompressPage->setdefaultpath(fileinfo.path());
            }

            loadArchive(files.at(0));
        }
        else {
            DDialog* dialog = new DDialog;
            dialog->setMessage(tr("添加压缩文件到目录或在新窗口中打开该文件？"));
            dialog->addButton(tr("取消"));
            dialog->addButton(tr("添加"));
            dialog->addButton(tr("在新窗口中打开"));
            const int mode = dialog->exec();
            qDebug()<<mode;
            if(1 == mode)
            {
                emit sigZipSelectedFiles(files);
            }
            else if(2 == mode)
            {
                KProcess* cmdprocess = new KProcess;
                QStringList arguments;

                QString programPath = QStandardPaths::findExecutable("deepin-compressor");
                if (programPath.isEmpty()) {
                    qDebug()<<"error can't find xdg-mime";
                    return;
                }

                arguments<<files.at(0);

                qDebug()<<arguments;

                cmdprocess->setOutputChannelMode(KProcess::MergedChannels);
                cmdprocess->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered | QIODevice::Text);
                cmdprocess->setProgram(programPath, arguments);
                cmdprocess->start();
            }
        }
    }
    else {
        m_pageid = PAGE_ZIP;
        emit sigZipSelectedFiles(files);
        refreshPage();
    }
}

void MainWindow::onRightMenuSelected(const QStringList &files)
{
    if(files.count() == 2 && files.at(1) == QStringLiteral("extract_here") && Utils::isCompressed_file(files.at(0)))
    {
        m_isrightmenu = true;
        QFileInfo fileinfo(files.at(0));
        m_decompressfilename = fileinfo.fileName();
        m_UnCompressPage->setdefaultpath(fileinfo.path());
        loadArchive(files.at(0));
        m_pageid = PAGE_UNZIPPROGRESS;
        m_Progess->settype(DECOMPRESSING);
        refreshPage();
    }
    else if(files.count() == 1 && Utils::isCompressed_file(files.at(0)))
    {
        QFileInfo fileinfo(files.at(0));
        m_decompressfilename = fileinfo.fileName();
        if("" != m_settingsDialog->getCurExtractPath())
        {
           m_UnCompressPage->setdefaultpath(m_settingsDialog->getCurExtractPath());
        }
        else
        {
           m_UnCompressPage->setdefaultpath(fileinfo.path());
        }

        loadArchive(files.at(0));
    }
    else {
        emit sigZipSelectedFiles(files);
        m_pageid = PAGE_ZIPSET;
        setCompressDefaultPath();
        refreshPage();
    }
}

void MainWindow::slotLoadingFinished(KJob *job)
{
    m_workstatus = WorkNone;
    if (job->error()) {
        m_CompressFail->setFailStrDetail(tr("压缩文件已损坏!"));
        m_pageid = PAGE_UNZIP_FAIL;
        refreshPage();
        return;
    }
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterKeyColumn(0);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_UnCompressPage->setModel(m_filterModel);


    m_homePage->spinnerStop();
    if(!m_isrightmenu)
    {
        m_pageid = PAGE_UNZIP;
        refreshPage();
    }
    else {
        slotextractSelectedFilesTo(m_UnCompressPage->getDecompressPath());
    }
}


void MainWindow::loadArchive(const QString &files)
{
    m_workstatus = WorkProcess;
    m_loadfile = files;
    m_encryptiontype = Encryption_Load;
    m_loadjob = (LoadJob*)m_model->loadArchive(files, "", m_model);

    connect(m_loadjob, &LoadJob::sigLodJobPassword,
            this, &MainWindow::SlotNeedPassword);

    if (m_loadjob) {
        m_loadjob->start();
        m_homePage->spinnerStart();
    }
}

void MainWindow::slotextractSelectedFilesTo(const QString& localPath)
{
    m_workstatus = WorkProcess;
    m_encryptiontype = Encryption_Extract;
    if (!m_model) {
        return;
    }

    if(m_encryptionjob)
    {
        m_encryptionjob = nullptr;
    }

    ExtractionOptions options;
    QVector<Archive::Entry*> files;

    QString userDestination = localPath;
    QString destinationDirectory;

    if(m_settingsDialog->isAutoCreatDir())
    {
        const QString detectedSubfolder = m_model->archive()->subfolderName();
        qDebug() << "Detected subfolder" << detectedSubfolder;

        if (m_model->archive()->hasMultipleTopLevelEntries()) {
            if (!userDestination.endsWith(QDir::separator())) {
                userDestination.append(QDir::separator());
            }
            destinationDirectory = userDestination + detectedSubfolder;
            QDir(userDestination).mkdir(detectedSubfolder);
        } else {
            destinationDirectory = userDestination;
        }
    }
    else {
        destinationDirectory = userDestination;
    }


    qDebug()<<"destinationDirectory:"<<destinationDirectory;
    m_encryptionjob = m_model->extractFiles(files, destinationDirectory, options);

    connect(m_encryptionjob, SIGNAL(percent(KJob*,ulong)),
            this, SLOT(SlotProgress(KJob*,ulong)));
    connect(m_encryptionjob, &KJob::result,
            this, &MainWindow::slotExtractionDone);
    connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
            this, &MainWindow::SlotNeedPassword, Qt::QueuedConnection);
    connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
            m_encryptionpage, &EncryptionPage::wrongPassWordSlot);
    connect(m_encryptionjob, SIGNAL(percentfilename(KJob*, const QString &)),
            this, SLOT(SlotProgressFile(KJob*, const QString &)));

    m_encryptionjob->start();
    m_decompressfilepath = destinationDirectory;

}

void MainWindow::SlotProgress(KJob *job, unsigned long percent)
{
    qDebug()<<percent;
    if((Encryption_SingleExtract == m_encryptiontype))
    {
        if((percent < 100) && (percent > 0) && (WorkProcess == m_workstatus))
        {
            if(!m_progressdialog->isshown())
            {
                m_pageid = PAGE_UNZIP;
                refreshPage();
                m_progressdialog->showdialog();
            }
            m_progressdialog->setProcess(percent);
        }
    }
    else if(PAGE_ZIPPROGRESS == m_pageid || PAGE_UNZIPPROGRESS == m_pageid)
    {
        m_Progess->setprogress(percent);
    }
    else if((PAGE_UNZIP == m_pageid || PAGE_ENCRYPTION == m_pageid) && (percent < 100) && (percent > 0))
    {
        m_pageid = PAGE_UNZIPPROGRESS;
        m_Progess->settype(DECOMPRESSING);
        refreshPage();
    }
    else if((PAGE_ZIPSET == m_pageid) && (percent < 100) && (percent > 0))
    {
        m_pageid = PAGE_ZIPPROGRESS;
        m_Progess->settype(COMPRESSING);
        refreshPage();
    }
}

void MainWindow::SlotProgressFile(KJob *job, const QString& filename)
{
    if(Encryption_SingleExtract == m_encryptiontype && PAGE_UNZIP == m_pageid)
    {
        m_progressdialog->setCurrentFile(filename);
    }
    else if(PAGE_ZIPPROGRESS == m_pageid || PAGE_UNZIPPROGRESS == m_pageid)
    {
        m_Progess->setProgressFilename(filename);
    }
}

void MainWindow::slotExtractionDone(KJob* job)
{
    m_workstatus = WorkNone;
    if(m_encryptionjob)
    {
        m_encryptionjob->deleteLater();
        m_encryptionjob = nullptr;
    }

    if((PAGE_ENCRYPTION == m_pageid) && (job->error() && job->error() != KJob::KilledJobError))
    {
        //do noting:wrong password
    }
    else if (job->error() && job->error() != KJob::KilledJobError) {
        m_CompressFail->setFailStrDetail(tr("压缩文件已损坏!"));
        m_pageid = PAGE_UNZIP_FAIL;
        refreshPage();
        return;
    }
    else if(Encryption_SingleExtract == m_encryptiontype)
    {
        m_progressdialog->setFinished(m_decompressfilepath);
    }
    else {
        m_pageid = PAGE_UNZIP_SUCCESS;
        refreshPage();
    }
}

void MainWindow::SlotNeedPassword()
{

    m_pageid = PAGE_ENCRYPTION;
    refreshPage();
}

void MainWindow::SlotExtractPassword(QString password)
{
    if(Encryption_Load == m_encryptiontype)
    {
        LoadPassword(password);
    }
    else if(Encryption_Extract == m_encryptiontype)
    {
        ExtractPassword(password);
    }
    else if(Encryption_SingleExtract == m_encryptiontype)
    {
        ExtractSinglePassword(password);
    }
}

void MainWindow::ExtractSinglePassword(QString password)
{
    m_workstatus = WorkProcess;
    if(m_encryptionjob)//first  time to extract
    {
        m_encryptionjob->archiveInterface()->setPassword(password);

        m_encryptionjob->start();

    }
    else //second or more  time to extract
    {
        ExtractionOptions options;

        m_encryptionjob = m_model->extractFiles(m_extractSimpleFiles, m_decompressfilepath, options);
        m_encryptionjob->archiveInterface()->setPassword(password);
        connect(m_encryptionjob, SIGNAL(percent(KJob*,ulong)),
                this, SLOT(SlotProgress(KJob*,ulong)));
        connect(m_encryptionjob, &KJob::result,
                this, &MainWindow::slotExtractionDone);
        connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
                this, &MainWindow::SlotNeedPassword);
        connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
                m_encryptionpage, &EncryptionPage::wrongPassWordSlot);
        connect(m_encryptionjob, SIGNAL(percentfilename(KJob*, const QString &)),
                this, SLOT(SlotProgressFile(KJob*, const QString &)));

        m_encryptionjob->start();
    }
}

void MainWindow::ExtractPassword(QString password)
{
    m_workstatus = WorkProcess;
    if(m_encryptionjob)//first  time to extract
    {
        m_encryptionjob->archiveInterface()->setPassword(password);

        m_encryptionjob->start();

    }
    else //second or more  time to extract
    {
        ExtractionOptions options;
        QVector<Archive::Entry*> files;

        m_encryptionjob = m_model->extractFiles(files, m_decompressfilepath, options);
        m_encryptionjob->archiveInterface()->setPassword(password);
        connect(m_encryptionjob, SIGNAL(percent(KJob*,ulong)),
                this, SLOT(SlotProgress(KJob*,ulong)));
        connect(m_encryptionjob, &KJob::result,
                this, &MainWindow::slotExtractionDone);
        connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
                this, &MainWindow::SlotNeedPassword);
        connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
                m_encryptionpage, &EncryptionPage::wrongPassWordSlot);
        connect(m_encryptionjob, SIGNAL(percentfilename(KJob*, const QString &)),
                this, SLOT(SlotProgressFile(KJob*, const QString &)));

        m_encryptionjob->start();
    }
}
void MainWindow::LoadPassword(QString password)
{
    m_workstatus = WorkProcess;
    m_encryptiontype = Encryption_Load;
    m_loadjob = (LoadJob*)m_model->loadArchive(m_loadfile, "", m_model);

    connect(m_loadjob, &LoadJob::sigWrongPassword,
            m_encryptionpage, &EncryptionPage::wrongPassWordSlot);

    m_loadjob->archiveInterface()->setPassword(password);
    if (m_loadjob) {
        m_loadjob->start();
    }
}

void MainWindow::setCompressDefaultPath()
{
    QString path;
    QStringList fileslist = m_CompressPage->getCompressFilelist();

    QFileInfo fileinfobase(fileslist.at(0));
    for(int loop = 1; loop < fileslist.count();loop++)
    {
        QFileInfo fileinfo(fileslist.at(loop));
        if(fileinfo.path() != fileinfobase.path())
        {
            m_CompressSetting->setDefaultPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
            return;
        }
    }

    m_CompressSetting->setDefaultPath(fileinfobase.path());
    if(1 == fileslist.count())
    {
        m_CompressSetting->setDefaultName(fileinfobase.baseName());
    }
    else {
        m_CompressSetting->setDefaultName(tr("新建归档文件"));
    }

}

void MainWindow::onCompressNext()
{
    m_pageid = PAGE_ZIPSET;
    setCompressDefaultPath();
    refreshPage();
}

void MainWindow::onCompressPressed(QMap<QString, QString> &Args)
{
    creatArchive(Args);
}

QString MainWindow::renameCompress(QString &filename, QString fixedMimeType)
{
    QString localname = filename;
    int num = 2;
    while(QFileInfo::exists(filename))
    {
        filename = localname.remove("." + QMimeDatabase().mimeTypeForName(fixedMimeType).preferredSuffix()) + "0" + QString::number(num) + "." + QMimeDatabase().mimeTypeForName(fixedMimeType).preferredSuffix();
        num++;
    }
    return filename;
}


void MainWindow::creatArchive(QMap<QString, QString> &Args)
{
    const QStringList filesToAdd = m_CompressPage->getCompressFilelist();
    const QString fixedMimeType = Args[QStringLiteral("fixedMimeType")];
    const QString password = Args[QStringLiteral("encryptionPassword")];
    const QString enableHeaderEncryption = Args[QStringLiteral("encryptHeader")];
    QString filename = Args[QStringLiteral("localFilePath")] + "/" + Args[QStringLiteral("filename")];
    m_decompressfilename = Args[QStringLiteral("filename")];
    m_CompressSuccess->setCompressPath(Args[QStringLiteral("localFilePath")]);

    if (filename.isEmpty()) {
        qDebug()<<"filename.isEmpty()";
        return;
    }

    renameCompress(filename, fixedMimeType);
    qDebug()<<filename;


    CompressionOptions options;
    options.setCompressionLevel(Args[QStringLiteral("compressionLevel")].toInt());
//    options.setCompressionMethod(Args[QStringLiteral("compressionMethod")]);
    options.setEncryptionMethod(Args[QStringLiteral("encryptionMethod")]);
    options.setVolumeSize(Args[QStringLiteral("volumeSize")].toULongLong());


    QVector<Archive::Entry*> all_entries;

    foreach(QString file, filesToAdd)
    {
        Archive::Entry *entry = new Archive::Entry();
        entry->setFullPath(file);
        all_entries.append(entry);
    }


    if (all_entries.isEmpty()) {
        qDebug()<<"all_entries.isEmpty()";
        return;
    }

    QString globalWorkDir = filesToAdd.first();
    if (globalWorkDir.right(1) == QLatin1String("/")) {
        globalWorkDir.chop(1);
    }
    globalWorkDir = QFileInfo(globalWorkDir).dir().absolutePath();
    options.setGlobalWorkDir(globalWorkDir);

    m_createJob = Archive::create(filename, fixedMimeType, all_entries, options, this);

    if (!password.isEmpty()) {
        m_createJob->enableEncryption(password, enableHeaderEncryption.compare("true")?false:true);
    }

    connect(m_createJob, &KJob::result, this, &MainWindow::slotCompressFinished);
    connect(m_createJob, SIGNAL(percent(KJob*,ulong)),
            this, SLOT(SlotProgress(KJob*,ulong)));
    connect(m_createJob, &CreateJob::percentfilename,
            this, &MainWindow::SlotProgressFile);
    m_createJob->start();
    m_workstatus = WorkProcess;

}

void MainWindow::slotCompressFinished(KJob *job)
{
    qDebug() << "job finished"<<job->error();
    m_workstatus = WorkNone;
    if (job->error() &&  job->error() != KJob::KilledJobError) {
        m_pageid = PAGE_ZIP_FAIL;
        m_CompressFail->setFailStrDetail(tr("原始文件已损坏！"));
        refreshPage();
        return;
    }
    m_pageid = PAGE_ZIP_SUCCESS;
    refreshPage();

//    if(m_createJob)
//    {
//        m_createJob->kill();
//    }

}

void MainWindow::slotExtractSimpleFiles(QVector<Archive::Entry*> fileList, QString path)
{
    m_workstatus = WorkProcess;
    m_encryptiontype = Encryption_SingleExtract;
    m_progressdialog->clearprocess();
    if (!m_model) {
        return;
    }

    if(m_encryptionjob)
    {
        m_encryptionjob = nullptr;
    }

    ExtractionOptions options;
    options.setDragAndDropEnabled(true);
    m_extractSimpleFiles = fileList;

    const QString destinationDirectory = path;
    qDebug()<<"destinationDirectory:"<<destinationDirectory;
    m_encryptionjob = m_model->extractFiles(fileList, destinationDirectory, options);

    connect(m_encryptionjob, SIGNAL(percent(KJob*,ulong)),
            this, SLOT(SlotProgress(KJob*,ulong)));
    connect(m_encryptionjob, &KJob::result,
            this, &MainWindow::slotExtractionDone);
    connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
            this, &MainWindow::SlotNeedPassword, Qt::QueuedConnection);
    connect(m_encryptionjob, &ExtractJob::sigExtractJobPassword,
            m_encryptionpage, &EncryptionPage::wrongPassWordSlot);
    connect(m_encryptionjob, SIGNAL(percentfilename(KJob*, const QString &)),
            this, SLOT(SlotProgressFile(KJob*, const QString &)));

    m_encryptionjob->start();
    m_decompressfilepath = destinationDirectory;

    QFileInfo file(m_loadfile);
    m_progressdialog->setCurrentTask(file.fileName());

}

void MainWindow::slotKillExtractJob()
{
    m_workstatus = WorkNone;
    if(m_encryptionjob)
    {
        m_encryptionjob->Killjob();
    }
}

void MainWindow::slotFailRetry()
{
    if(Encryption_Load == m_encryptiontype)
    {
        m_pageid = PAGE_HOME;
        refreshPage();
        loadArchive(m_loadfile);
    }
    else if(Encryption_Extract == m_encryptiontype)
    {
        slotextractSelectedFilesTo(m_UnCompressPage->getDecompressPath());
    }
    else if(Encryption_SingleExtract == m_encryptiontype)
    {

    }
}

void MainWindow::onCancelCompressPressed()
{
    if(m_encryptionjob)
    {
        m_encryptionjob->Killjob();
    }
    if(m_createJob)
    {
        m_createJob->kill();
    }

    emit sigquitApp();
}

void MainWindow::onTitleButtonPressed()
{
    switch (m_pageid) {
    case PAGE_ZIP:
        emit sigZipAddFile();
        break;
    case PAGE_ZIPSET:
        emit sigZipReturn();
        m_pageid = PAGE_ZIP;
        refreshPage();
        break;
    default:
        break;
    }
    return;
}

