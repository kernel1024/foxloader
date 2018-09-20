#include <QUrl>
#include <QMenu>
#include <QStandardPaths>
#include <QDir>
#include <QWebEngineSettings>
#include <QWebEngineCookieStore>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    nam = new QNetworkAccessManager(this);
    ui->web->parentWindow = this;

    loadingActive = false;
    capturing = false;
    mainCounter = -1;

    webProfile = new QWebEngineProfile("foxloader",this);
    QString fs = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (fs.isEmpty()) fs = QDir::homePath() + QDir::separator() + QStringLiteral(".cache")
                           + QDir::separator() + QStringLiteral("foxloader");
    if (!fs.endsWith(QDir::separator())) fs += QDir::separator();
    QString fcache = fs + QStringLiteral("cache") + QDir::separator();
    webProfile->setCachePath(fcache);
    QString fdata = fs + QStringLiteral("local_storage") + QDir::separator();
    webProfile->setPersistentStoragePath(fdata);

    webProfile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    webProfile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);
    webProfile->setSpellCheckEnabled(false);

    webProfile->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled,true);
    webProfile->settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage,true);



    connect(ui->btnGo,&QPushButton::clicked,[this](){
        ui->web->load(QUrl::fromUserInput(ui->editUrl->text()));
    });

    connect(ui->web,&QWebEngineView::urlChanged,[this](const QUrl& url){
        ui->editUrl->setText(url.toString());
    });

    connect(ui->web,&QWebEngineView::titleChanged,[this](const QString& title){
        setWindowTitle(QString("FoxLoader - %1").arg(title.left(100)));
    });

    connect(webProfile->cookieStore(), &QWebEngineCookieStore::cookieAdded,[this](const QNetworkCookie &cookie){
        if (nam->cookieJar()!=nullptr)
            nam->cookieJar()->insertCookie(cookie);
    });

    connect(webProfile->cookieStore(), &QWebEngineCookieStore::cookieRemoved,[this](const QNetworkCookie &cookie){
        if (nam->cookieJar()!=nullptr)
            nam->cookieJar()->deleteCookie(cookie);
    });

    connect(ui->web, &QWebEngineView::loadProgress,[this](int progress){
        ui->webLoading->setValue(progress);
    });

    connect(ui->web, &QWebEngineView::loadFinished,[this](bool ok){
        ui->webLoading->setValue(100);
        if (loadingActive) {
            if (ok)
                handlePage();
            else
                stopLoading();
        }
    });

    connect(ui->web, &QWebEngineView::loadStarted,[this](){
        ui->webLoading->setValue(0);
    });

    connect(ui->web->page(), &QWebEnginePage::linkHovered,[this](const QUrl& url){
        ui->statusBar->showMessage(url.toString());
    });

    connect(ui->btnCapture, &QPushButton::clicked,[this](){
        ui->spinX->setValue(0);
        ui->spinY->setValue(0);
        ui->statusBar->showMessage("Please RIGHT click mouse in browser");
        addLog("Please RIGHT click mouse in browser");
        capturing = true;
    });

    connect(ui->btnDir, &QPushButton::clicked,[this](){
        QString s = QFileDialog::getExistingDirectory(this,QString("Save files to"),
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        if (!s.isEmpty())
            ui->editDir->setText(s);
    });

    connect(ui->btnStart, &QPushButton::clicked, this, &MainWindow::startLoading);
    connect(ui->btnStop, &QPushButton::clicked, this, &MainWindow::stopLoading);

    webProfile->cookieStore()->loadAllCookies();
    ui->btnStop->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleCtxMenu(const QPoint &pos, const QWebEngineContextMenuData &data)
{
    QUrl linkUrl, imageUrl;
    if (data.isValid()) {
        linkUrl = data.linkUrl();;
        if (data.mediaType()==QWebEngineContextMenuData::MediaTypeImage)
            imageUrl = data.mediaUrl();
    }
    QStringList imgs;
    imgs << "jpg" << "jpeg" << "png" << "bmp" << "gif";

    if (capturing) {
        ui->statusBar->clearMessage();
        ui->spinX->setValue(pos.x());
        ui->spinY->setValue(pos.y());
        scrollCaptured = ui->web->page()->scrollPosition();
        addLog("Mouse click captured");
        capturing = false;

    } else if (loadingActive) {

        QString link = linkUrl.toString();
        bool linkToImg = false;
        foreach (const QString& suffix, imgs) {
            if (link.endsWith(suffix, Qt::CaseInsensitive)) {
                linkToImg = true;
                break;
            }
        }

        if (!linkToImg) {
            if (imageUrl.isValid())
                linkUrl = imageUrl;
            else
                linkUrl = QUrl();
        }
        if (!linkUrl.isEmpty() && linkUrl.isValid()) {
            QNetworkReply* rep = nam->get(QNetworkRequest(linkUrl));
            int cnt = mainCounter;
            connect(rep,&QNetworkReply::finished,[this,rep,cnt](){
                saveFile(cnt,rep->url(),rep->readAll());
                rep->deleteLater();
            });
        } else {
            addLog(QString("Skipping image - not found. At %1").arg(mainCounter));
        }

        mainCounter++;
        if (mainCounter>ui->spinStop->value())
            stopLoading();
        else
            performLoad();

    } else {

        QMenu *menu = ui->web->page()->createStandardContextMenu();
        menu->popup(ui->web->mapToGlobal(pos));
    }
}

void MainWindow::performLoad()
{
    QString tmpl = ui->editTemplate->text();
    QRegExp rx("@{1,}");

    int pos = rx.indexIn(tmpl);
    if (pos<0) {
        stopLoading();
        addLog("Url doesn't contains counter template!");
        return;
    }
    int len = rx.matchedLength();
    QString counter;
    if (len==1)
        counter = QString("%1").arg(mainCounter);
    else
        counter = QString("%1").arg(mainCounter,len,10,QLatin1Char('0'));

    tmpl.replace(pos,len,counter);

    QUrl url(tmpl);
    if (url.isValid() && !url.isRelative() && !url.isLocalFile()) {
        ui->web->load(url);
    } else {
        stopLoading();
        addLog(QString("Incorrect url processed, aborting - '%1'").arg(tmpl));
        return;
    }
}

void MainWindow::saveFile(int idx, const QUrl& origin, const QByteArray &data)
{
    QDir savePath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (!ui->editDir->text().isEmpty()) {
        QDir dir(ui->editDir->text());
        if (dir.exists() && dir.isReadable())
            savePath = dir;
    }

    QString path = QFileInfo(origin.path()).fileName();
    int numlen = QString("%1").arg(ui->spinStop->value()).length();
    QString fname = QString("%1_%2")
                    .arg(idx,numlen,10,QLatin1Char('0'))
                    .arg(path);
    QFile f(savePath.absoluteFilePath(fname));
    if (!f.open(QIODevice::WriteOnly)) {
        addLog(QString("Unable to write file %1").arg(f.fileName()));
        return;
    }
    f.write(data);
    f.close();
    addLog(QString("File saved - %1").arg(f.fileName()));
}

void MainWindow::handlePage()
{
    if (!loadingActive) return;

    if (!scrollCaptured.isNull()) {
        QString js = QString("window.scrollTo(%1, %2);")
                     .arg(qRound(scrollCaptured.x()))
                     .arg(qRound(scrollCaptured.y()));
        ui->web->page()->runJavaScript(js);
    }

    qApp->processEvents();

    QPoint p(ui->spinX->value(),ui->spinY->value()); // webEngine coords
    p = ui->web->mapToGlobal(p);

    QWidget* eventsReciverWidget = nullptr;
    foreach(QObject* obj, ui->web->children())
    {
        QWidget* wgt = qobject_cast<QWidget*>(obj);
        if (wgt)
        {
            eventsReciverWidget = wgt;
            break;
        }
    }

    if (eventsReciverWidget!=nullptr)
        sendRightClickToWidget(eventsReciverWidget,eventsReciverWidget->mapFromGlobal(p));
}

void MainWindow::startLoading()
{
    if (capturing || loadingActive) return;

    loadingActive = true;
    ui->btnStart->setEnabled(false);
    ui->btnStop->setEnabled(true);
    mainCounter = ui->spinStart->value();

    performLoad();
}

void MainWindow::stopLoading()
{
    loadingActive = false;
    ui->btnStart->setEnabled(true);
    ui->btnStop->setEnabled(false);
    ui->web->stop();
    mainCounter = -1;
}

void MainWindow::addLog(const QString &msg)
{
    ui->textLog->appendPlainText(msg);
}
