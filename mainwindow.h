#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMimeData>
#include <qmessagebox.h>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>
#include <QProcess>
#include <QDragEnterEvent>
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool isVideoFile(const QString &filePath);
    void getVideoDuration(const QString &filePath);
    void getVideoFrameRate(const QString &filePath);
private:
    Ui::MainWindow *ui;

protected:
    // 重写父类事件方法
    // 当拖拽对象进入窗口时调用
    void dragEnterEvent(QDragEnterEvent* event) override;
    // 当拖放对象在窗口内释放时调用
    void dropEvent(QDropEvent* event) override;
private slots:
    void on_horizontalSlider_valueChanged(int value);
    void on_horizontalSlider_sliderReleased();
};
#endif // MAINWINDOW_H
