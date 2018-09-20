#ifndef SPECWIDGETS_H
#define SPECWIDGETS_H

#include <QWebEngineView>

class CWebEngineView : public QWebEngineView
{
public:
    QWidget* parentWindow;
    CWebEngineView(QWidget* parent = nullptr);
protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type);
    void contextMenuEvent(QContextMenuEvent *event);
};

void sendStringToWidget(QWidget *widget, const QString& s);
void sendRightClickToWidget(QWidget *window, const QPoint& pos);
void sendLeftClickToWidget(QWidget *window, const QPoint& pos);
void sendKeyboardInputToView(QWidget *widget, const QString& s);

#endif // SPECWIDGETS_H
