
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setAcceptDrops(true);// 开启对整个窗口的拖放操作的支持

    // ImageLabel *label = new ImageLabel("C:/Users/lwm/Desktop/1.png", this);
    // setCentralWidget(label);

    // MyDisplayImgWidget *w = new MyDisplayImgWidget(this);
    // setCentralWidget(w);
    // QPushButton *p = new QPushButton(this);
    // p->move(50,50);
    // connect(p,&QPushButton::clicked,[=](){
    //     w->setFileInfo(QFileInfo("C:/Users/lwm/Desktop/1.png"));
    // });

    //初始化QSettings
    QString appDirPath = QCoreApplication::applicationDirPath();// 获取exe所在目录
    QString configFilePath = appDirPath + "/config.ini";
    // 检查配置文件是否存在
    // QFile::exists(config_path);
    // 初始化QSettings（即使文件不存在，QSettings也会在写入时自动创建）
    m_settings = new QSettings(configFilePath, QSettings::IniFormat);
    // 设置组织名和应用名（影响INI文件的[General]部分）
    m_settings->setFallbacksEnabled(false); // 禁用回退到系统注册表

    readSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::isVideoFile(const QFileInfo &fileInfo)
{
    QStringList videoExtensions = {
        "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm",
        "m4v", "3gp", "mpeg", "mpg", "ts", "mts"
    };

    QString suffix = fileInfo.suffix().toLower();
    return videoExtensions.contains(suffix);
}

void MainWindow::getVideoFrameRate(const QFileInfo &fileInfo)
{
    // 构建并执行命令 - 获取视频流的帧率
    /*单例异步*/
    QString program = "ffprobe";
    QStringList arguments;
    arguments << "-v" << "error"// 参数1：只输出错误信息，屏蔽普通日志/警告
              << "-select_streams" << "v:0"// 参数2：只选择第1个视频流（v:0），忽略音频/字幕流
              << "-show_entries" << "stream=r_frame_rate"// 参数3：只显示视频流的r_frame_rate字段（帧率）
              << "-of" << "default=noprint_wrappers=1:nokey=1"// 参数4：输出格式配置，只打印值、不打印键和包装信息
              << fileInfo.absoluteFilePath();// 参数5：目标视频文件的路径
    QString workingDir = "";

    QProcess *process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(arguments);
    if (!workingDir.isEmpty())
        process->setWorkingDirectory(workingDir);
    // 连接信号槽
    // connect(process, &QProcess::readyReadStandardOutput, [=]() {
    //     qDebug() << "stdOutput:" << process->readAllStandardOutput();
    // });
    // connect(process, &QProcess::readyReadStandardError, [=]() {
    //     qDebug() << "stdError:" << process->readAllStandardError();
    // });
    connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        qDebug() << "Error occurred:" << error;
    });
    connect(process, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            // qDebug() << "Finished:" << process->readAllStandardOutput();
            QString output = process->readAllStandardOutput();
            qDebug() << "Finished:从QProcess返回的帧率：" << output;

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
            qDebug() << "Exited,Error occurred:" << process->readAllStandardError();
        }
        process->deleteLater();
    });
    process->start();

    //getVideoDuration必须要等获取帧数完成，好吧，必须改成阻塞才行。
    process->waitForFinished(5000);

}

void MainWindow::getVideoDuration(const QFileInfo &fileInfo)
{
    /*单例异步*/
    QString program = "ffprobe";
    QStringList arguments;
    arguments << "-v" << "error"// 参数1：日志级别，只输出错误信息，屏蔽普通日志/警告
              << "-show_entries" << "format=duration"// 参数2：指定提取format域下的duration字段（总时长）
              << "-of" << "default=noprint_wrappers=1:nokey=1"// 参数3：输出格式控制，仅打印数值
              << fileInfo.absoluteFilePath();// 参数4：目标视频文件的路径
    QString workingDir = "";

    QProcess *process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(arguments);
    if (!workingDir.isEmpty())
        process->setWorkingDirectory(workingDir);
    // 连接信号槽
    // connect(process, &QProcess::readyReadStandardOutput, [=]() {
    //     qDebug() << "stdOutput:" << process->readAllStandardOutput();
    // });
    // connect(process, &QProcess::readyReadStandardError, [=]() {
    //     qDebug() << "stdError:" << process->readAllStandardError();
    // });
    connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        qDebug() << "Error occurred:" << error;
    });
    connect(process, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            // qDebug() << "Finished:" << process->readAllStandardOutput();
            QString output = process->readAllStandardOutput();
            qDebug() << "Finished:从QProcess返回视频时长(s)：" << output;

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
            qDebug() << "Exited,Error occurred:" << process->readAllStandardError();
        }
        process->deleteLater();
    });
    process->start();

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

void MainWindow::getVideoCurrentFrameFile(const QFileInfo &fileInfo)
{
    //滑块释放事件，调用预览缩略图QProcess

    if (!fileInfo.exists())
    {
        //QMessageBox::warning(this, "warning", "输入栏为空, 不执行命令");

        return ;
    }
    QString currentTime = ui->current_time->time().toString("hh:mm:ss.zzz");

    qDebug()<<"获取帧的时间为："<<currentTime;

    /*单例异步*/
    QString program = "ffmpeg";
    QStringList arguments;
    arguments << "-y"// 添加覆盖指令，自动覆盖输出文件
              << "-ss" << currentTime
              << "-i" << fileInfo.absoluteFilePath()
              << "-frames:v" << "1" << "current_frame.png";
    QString workingDir = "";

    QProcess *process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(arguments);
    if (!workingDir.isEmpty())
        process->setWorkingDirectory(workingDir);
    // 连接信号槽
    // connect(process, &QProcess::readyReadStandardOutput, [=]() {
    //     qDebug() << "stdOutput:" << process->readAllStandardOutput();
    // });
    // connect(process, &QProcess::readyReadStandardError, [=]() {
    //     qDebug() << "stdError:" << process->readAllStandardError();
    // });
    connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        qDebug() << "Error occurred:" << error;
    });
    connect(process, &QProcess::finished, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
            qDebug() << "Finished:" << process->readAllStandardOutput();

            ui->widget->setFileInfo(QFileInfo("current_frame.png"));

        } else {
            qDebug() << "Exited,Error occurred:" << process->readAllStandardError();
        }
        process->deleteLater();
    });
    process->start();

}

void MainWindow::readSettings()
{
    const auto geometry = m_settings->value("geometry", QByteArray()).toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
}

void MainWindow::writeSettings()
{
    // 将“窗口位置和大小”写入ini
    m_settings->setValue("geometry", saveGeometry());
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
            // ui->fileInfo->clear();
            // 报错
            QMessageBox::warning(this, "剪辑", "请拖入单个视频文件");
        }
        return;
    }

    const QUrl& url = urls.first();
    QString filePath = url.toLocalFile();

    // 检查文件是否存在且是视频文件
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !isVideoFile(fileInfo)) {
        if(fileInfo.suffix().toLower()=="png"){
            qDebug()<<"拖入png文件，忽略提示";
        }else{
            QMessageBox::warning(this, "剪辑", "请拖入有效的视频文件");
        }
        return;
    }

    // 获取 “文件地址”，显示到控件
    m_videoFileInfo = fileInfo;
    setWindowTitle(m_videoFileInfo.absoluteFilePath());

    // 获取 “视频帧率”，显示到控件
    getVideoFrameRate(fileInfo);//必须阻塞

    // 获取 “总时长”，显示到控件
    getVideoDuration(fileInfo);

    // 渲染视频第0帧，显示到控件
    ui->horizontalSlider->setValue(0);
    getVideoCurrentFrameFile(m_videoFileInfo);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // 关闭窗口时，将“窗口位置和大小”写入ini
    writeSettings();
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    qDebug()<<"on_horizontalSlider_valueChanged(int value)";
    {
        const QSignalBlocker blocker1(ui->current_frame);
        const QSignalBlocker blocker2(ui->current_time);
        // no signals here

        //值变化的时候，同步更新到当前时间、当前帧数控件，但不触发他们的槽函数

        ui->current_frame->setValue(value);

        int current_frame = value;
        double current_second = current_frame / ui->video_frame_rate->value() ;
        QTime current_time = convertToTime(current_second);
        // 更新timeEdit控件显示总时长
        ui->current_time->setTime(current_time);
    }
}


void MainWindow::on_horizontalSlider_sliderReleased()
{
    qDebug()<<"on_horizontalSlider_sliderReleased()";
    getVideoCurrentFrameFile(m_videoFileInfo);
}


void MainWindow::on_current_frame_valueChanged(int arg1)
{
    qDebug()<<"on_current_frame_valueChanged(int arg1)";

    {
        const QSignalBlocker blocker1(ui->horizontalSlider);
        const QSignalBlocker blocker2(ui->current_time);
        // no signals here

        //值变化的时候，同步更新到当前时间、当前帧数控件，但不触发他们的槽函数

        ui->horizontalSlider->setValue(arg1);

        int current_frame = arg1;
        double current_second = current_frame / ui->video_frame_rate->value() ;
        QTime current_time = convertToTime(current_second);
        // 更新timeEdit控件显示总时长
        ui->current_time->setTime(current_time);
    }
}


void MainWindow::on_current_time_timeChanged(const QTime &time)
{
    qDebug()<<"on_current_time_timeChanged(const QTime &time)";

    {
        const QSignalBlocker blocker1(ui->horizontalSlider);
        const QSignalBlocker blocker2(ui->current_frame);
        // no signals here

        //值变化的时候，同步更新到当前时间、当前帧数控件，但不触发他们的槽函数

        int current_frame = convertToSeconds(time) * ui->video_frame_rate->value() ;

        ui->horizontalSlider->setValue(current_frame);
        ui->current_frame->setValue(current_frame);
    }
}

