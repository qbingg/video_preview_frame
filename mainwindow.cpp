#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setAcceptDrops(true);// 开启对整个窗口的拖放操作的支持
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::isVideoFile(const QString &filePath)
{
    QStringList videoExtensions = {
        "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm",
        "m4v", "3gp", "mpeg", "mpg", "ts", "mts"
    };

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    return videoExtensions.contains(suffix);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction(); // 接受默认的拖放行为
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();

    // 确保文件数量仅为一个
    if (urls.size() != 1) {
        if (urls.size() > 1) {
            // 清空输入栏
            ui->fileInfo->clear();
            // 报错
            QMessageBox::warning(this, "剪辑", "请拖入单个视频文件");
        }
        return;
    }

    const QUrl& url = urls.first();
    QString filePath = url.toLocalFile();

    // 检查文件是否存在且是视频文件
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !isVideoFile(filePath)) {
        QMessageBox::warning(this, "剪辑", "请拖入有效的视频文件");
        return;
    }

    // 获取 “文件地址”，显示到控件
    ui->fileInfo->setText(filePath);

    // 获取 “视频帧率”，显示到控件
    getVideoFrameRate(filePath);

    // 获取 “总时长”，显示到控件
    getVideoDuration(filePath);
}

// 在实现文件中定义方法
void MainWindow::getVideoFrameRate(const QString& filePath)
{
    QProcess *process = new QProcess(this);

    // 连接信号，处理进程完成
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process, filePath](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                    QString output = process->readAllStandardOutput();
                    output = output.trimmed(); // 去除换行和空格

                    //从QProcess返回的帧率： "60/1"
                    qDebug()<<"从QProcess返回的帧率："<< output;

                    // 帧率可能以分数形式出现（如 30000/1001），需要解析
                    if (output.contains('/')) {
                        QStringList parts = output.split('/');
                        if (parts.size() == 2) {
                            bool numOk, denOk;
                            double numerator = parts[0].toDouble(&numOk);
                            double denominator = parts[1].toDouble(&denOk);

                            if (numOk && denOk && denominator != 0) {
                                double frameRate = numerator / denominator;
                                ui->video_frame_rate->setValue(frameRate);
                                qDebug() << "视频帧率:" << frameRate << "fps";
                            } else {
                                QMessageBox::warning(this, "错误", "无法解析帧率分数: " + output);
                            }
                        } else {
                            QMessageBox::warning(this, "错误", "帧率格式不正确: " + output);
                        }
                    } else {
                        // 直接是数字格式
                        bool ok;
                        double frameRate = output.toDouble(&ok);
                        if (ok && frameRate > 0) {
                            ui->video_frame_rate->setValue(frameRate);
                            qDebug() << "视频帧率:" << frameRate << "fps";
                        } else {
                            QMessageBox::warning(this, "错误", "无法解析帧率: " + output);
                        }
                    }
                } else {
                    QString error = process->readAllStandardError();
                    QMessageBox::warning(this, "错误", "获取视频帧率失败:\n" + error);
                }

                process->deleteLater();
            });

    // 连接错误处理
    connect(process, &QProcess::errorOccurred,
            [this, process](QProcess::ProcessError error) {
                QMessageBox::warning(this, "错误", "执行ffprobe获取帧率时发生错误");
                process->deleteLater();
            });

    // 构建并执行命令 - 获取视频流的帧率
    QStringList arguments;
    arguments << "-v" << "error"
              << "-select_streams" << "v:0"
              << "-show_entries" << "stream=r_frame_rate"
              << "-of" << "default=noprint_wrappers=1:nokey=1"
              << filePath;

    qDebug() << "执行帧率命令: ffprobe" << arguments;
    process->start("ffprobe", arguments);

    // 可选：设置超时（10秒）
    QTimer::singleShot(10000, process, [process]() {
        if (process->state() == QProcess::Running) {
            process->kill();
        }
    });
}

double MainWindow::convertToSeconds(const QTime time)
{
    // 获取毫秒精度的总秒数
    double totalSeconds = time.hour() * 3600.0 +
                          time.minute() * 60.0 +
                          time.second() +
                          time.msec() / 1000.0;
    QString str = QString::number(totalSeconds,'f',3);
    qDebug()<<"convertToSeconds："<<str;
    return totalSeconds;
}

QTime MainWindow::convertToTime(const double totalSeconds)
{
    // 将总秒数转换为时、分、秒、毫秒
    int hours = static_cast<int>(totalSeconds) / 3600;
    int minutes = (static_cast<int>(totalSeconds) % 3600) / 60;
    int seconds = static_cast<int>(totalSeconds) % 60;
    int milliseconds = static_cast<int>((totalSeconds - static_cast<int>(totalSeconds)) * 1000);

    // 创建 QTime 对象
    QTime time(hours, minutes, seconds, milliseconds);

    return time;
}

// 使用QProcess获取视频时长
void MainWindow::getVideoDuration(const QString& filePath)
{
    QProcess *process = new QProcess(this);

    // 连接信号，处理进程完成
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, process, filePath](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                    QString output = process->readAllStandardOutput();
                    output = output.trimmed(); // 去除换行和空格

                    bool ok;
                    double duration = output.toDouble(&ok);
                    if (ok && duration > 0) {

                        //设定值Slider最大值：帧数 = 总秒数*帧率
                        ui->horizontalSlider->setMaximum(duration * ui->video_frame_rate->value());

                        qDebug() << "视频时长:" << duration << "秒";
                    } else {
                        QMessageBox::warning(this, "错误", "无法解析视频时长: " + output);
                    }
                } else {
                    QString error = process->readAllStandardError();
                    QMessageBox::warning(this, "错误", "获取视频时长失败:\n" + error);
                }

                process->deleteLater();
            });

    // 连接错误处理
    connect(process, &QProcess::errorOccurred,
            [this, process](QProcess::ProcessError error) {
                QMessageBox::warning(this, "错误", "执行ffprobe时发生错误");
                process->deleteLater();
            });

    // 构建并执行命令
    QStringList arguments;
    arguments << "-v" << "error"
              << "-show_entries" << "format=duration"
              << "-of" << "default=noprint_wrappers=1:nokey=1"
              << filePath;

    qDebug() << "执行命令: ffprobe" << arguments;
    process->start("ffprobe", arguments);

    // 可选：设置超时（10秒）
    QTimer::singleShot(10000, process, [process]() {
        if (process->state() == QProcess::Running) {
            process->kill();
        }
    });
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    //值变化的时候，同步更新到当前时间、当前帧数控件，但不触发刷新缩略图QProcess

    ui->current_frame->setValue(value);

    int current_frame = value;
    double current_second = current_frame / ui->video_frame_rate->value() ;
    QTime current_time = convertToTime(current_second);
    // 更新timeEdit控件显示总时长
    ui->current_time->setTime(current_time);

}


void MainWindow::on_horizontalSlider_sliderReleased()
{
    //滑块释放事件，调用预览缩略图QProcess

    if (ui->fileInfo->text().isEmpty())
    {
        //QMessageBox::warning(this, "warning", "输入栏为空, 不执行命令");

        return ;
    }

    QLabel* startPng= ui->label_preview;

    if (true)
    {
        std::filesystem::path filePath = "current_frame.png";
        if (std::filesystem::exists(filePath)) {
            if (std::filesystem::remove(filePath.c_str()) != 0) {
                //std::cerr << "Error deleting file" << std::endl;
                //return -1;
            }
            //std::cout << "File deleted successfully" << std::endl;
        }
        else {
            //std::cout << "File does not exist" << std::endl;
        }

        // 获取文件名
        QFileInfo fileInfo(ui->fileInfo->text());// 完整路径/+输入文件名.webm

        // 当前时间点，秒数
        double currentFrame = ui->current_frame->value();
        double videoFrameRate = ui->video_frame_rate->value();
        QString currentTime = QString::number(( currentFrame / videoFrameRate), 'd', 5);

        QProcess *ffmpegProcess = new QProcess(this);
        //连接 finished 信号
        //使用 Lambda 表达式处理信号
        connect(ffmpegProcess, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                qDebug() << "图片png!";
                //QMessageBox::information(this, "", "图片png");

                // 加载图片
                QImage image;

                image.load("current_frame.png"); // 替换为你的图片路径

                // 将QImage转换为QPixmap
                QPixmap pixmap = QPixmap::fromImage(image);

                QPixmap show = pixmap.scaled(QSize(ui->label_preview->size()), Qt::KeepAspectRatio);

                // 在QLabel上显示图片
                startPng->setPixmap(show);

                startPng->show();

            } else {
                qDebug() << "Error occurred:" << ffmpegProcess->readAllStandardError();
            }
            ffmpegProcess->deleteLater();
        });

        connect(ffmpegProcess, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
                [=](QProcess::ProcessError error) {
                    qDebug() << "Process error:" << error;
                    ffmpegProcess->deleteLater();
                });

        // 构建并执行命令
        QStringList arguments;
        // arguments << "-ss" << startTime
        //           << "-i" << fileInfo.absoluteFilePath()
        //           << "-frames:v 1 startPng.png";// 错误：多个参数合并为一个
        arguments << "-ss" << currentTime
                  << "-i" << fileInfo.absoluteFilePath()
                  << "-frames:v" << "1" << "current_frame.png";

        qDebug() << "执行命令: ffmpeg" << arguments;
        ffmpegProcess->start("ffmpeg", arguments);




    }


}

