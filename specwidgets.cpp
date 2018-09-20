#include <QContextMenuEvent>
#include <QTest>
#include "specwidgets.h"
#include "mainwindow.h"

CWebEngineView::CWebEngineView(QWidget *parent)
    : QWebEngineView(parent)
{
    parentWindow = nullptr;

}

QWebEngineView *CWebEngineView::createWindow(QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type)

    return nullptr;
}

void CWebEngineView::contextMenuEvent(QContextMenuEvent *event)
{
    MainWindow* window = qobject_cast<MainWindow *>(parentWindow);
    if (window!=nullptr)
        window->handleCtxMenu(event->pos(), page()->contextMenuData());
}

void sendStringToWidget(QWidget *widget, const QString& s)
{
    if (widget==nullptr) return;
    for (int i=0;i<s.length();i++) {
        Qt::KeyboardModifiers mod = Qt::NoModifier;
        const QChar c = s.at(i);
        if (c.isLetter() && c.isUpper()) mod = Qt::ShiftModifier;
        QTest::sendKeyEvent(QTest::KeyAction::Click,widget,Qt::Key_A,c,mod);
    }
}

void sendRightClickToWidget(QWidget *window, const QPoint& pos)
{
    if (window==nullptr) return;

    Qt::KeyboardModifiers mod = Qt::NoModifier;
    QTest::mouseClick(window,Qt::RightButton,mod,pos);
}

void sendLeftClickToWidget(QWidget *window, const QPoint& pos)
{
    if (window==nullptr) return;

    Qt::KeyboardModifiers mod = Qt::NoModifier;
    QTest::mouseClick(window,Qt::LeftButton,mod,pos);
}

void sendKeyboardInputToView(QWidget *widget, const QString& s)
{
    if (widget==nullptr) return;

    widget->setFocus();
    Q_FOREACH(QObject* obj, widget->children()) {
        QWidget *w = qobject_cast<QWidget*>(obj);
        sendStringToWidget(w,s);
    }
    sendStringToWidget(widget,s);
}
