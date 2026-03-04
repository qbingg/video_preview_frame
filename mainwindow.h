#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <qmessagebox.h>
#include <QFileInfo>
#include <QMainWindow>
#include "MyDisplayImgWidget.h"
#include <QUrl>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QDebug>
#include <QMessageBox>
#include <QTime>
#include <QSignalBlocker>
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

    bool isVideoFile(const QFileInfo &fileInfo);
    void getVideoFrameRate(const QFileInfo& fileInfo);
    void getVideoDuration(const QFileInfo& fileInfo);
    double convertToSeconds(const QTime time);
    QTime convertToTime(const double totalSeconds);
    void getVideoCurrentFrameFile(const QFileInfo &fileInfo);
private:
    Ui::MainWindow *ui;

    QFileInfo m_videoFileInfo;

    // QPixmap m_currentFrame;

protected:
    // 重写父类事件方法
    // 当拖拽对象进入窗口时调用
    void dragEnterEvent(QDragEnterEvent* event) override;
    // 当拖放对象在窗口内释放时调用
    void dropEvent(QDropEvent* event) override;

private slots:
    void on_horizontalSlider_valueChanged(int value);
    void on_horizontalSlider_sliderReleased();
    void on_current_frame_valueChanged(int arg1);
    void on_current_time_timeChanged(const QTime &time);
};
#endif // MAINWINDOW_H
