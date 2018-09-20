#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QWebEngineProfile>
#include <QWebEngineContextMenuData>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void handleCtxMenu(const QPoint &pos, const QWebEngineContextMenuData& data);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager* nam;
    QWebEngineProfile* webProfile;

    bool loadingActive;
    bool capturing;
    QPointF scrollCaptured;
    int mainCounter;

    void performLoad();
    void saveFile(int idx, const QUrl &origin, const QByteArray& data);



public slots:
    void handlePage();
    void startLoading();
    void stopLoading();
    void addLog(const QString& msg);

};

#endif // MAINWINDOW_H
