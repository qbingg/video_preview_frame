import sys
import os
import subprocess
from pathlib import Path
from PySide6.QtWidgets import (QApplication, QMainWindow, QMessageBox, 
                              QFileDialog, QLabel)
from PySide6.QtCore import QTimer, QProcess, QTime, QSize
from PySide6.QtGui import QDragEnterEvent, QDropEvent, QPixmap, QImage
from PySide6.QtUiTools import QUiLoader
from PySide6.QtCore import QTimer, QProcess, QTime, QSize, Qt
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        # 动态加载UI文件
        self.ui = self.load_ui("mainwindow.ui")
        self.setCentralWidget(self.ui)
        
        # 初始化UI
        self.setup_ui()
        
    def load_ui(self, ui_file):
        loader = QUiLoader()
        file_path = Path(__file__).parent / ui_file
        if not file_path.exists():
            raise FileNotFoundError(f"UI文件不存在: {file_path}")
        return loader.load(str(file_path))
    
    def setup_ui(self):
        # 开启拖放支持
        self.setAcceptDrops(True)
        
        # 连接信号槽
        self.ui.horizontalSlider.valueChanged.connect(self.on_horizontalSlider_valueChanged)
        self.ui.horizontalSlider.sliderReleased.connect(self.on_horizontalSlider_sliderReleased)
        
        # 初始化预览标签
        self.preview_label = self.ui.label_preview
        self.preview_label.setScaledContents(False)
        self.preview_label.setAlignment(Qt.AlignCenter)
    
    def isVideoFile(self, file_path):
        video_extensions = {
            "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm",
            "m4v", "3gp", "mpeg", "mpg", "ts", "mts"
        }
        suffix = Path(file_path).suffix.lower().lstrip('.')
        return suffix in video_extensions
    
    def dragEnterEvent(self, event: QDragEnterEvent):
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
    
    def dropEvent(self, event: QDropEvent):
        urls = event.mimeData().urls()
        
        # 确保文件数量仅为一个
        if len(urls) != 1:
            if len(urls) > 1:
                self.ui.fileInfo.clear()
                QMessageBox.warning(self, "剪辑", "请拖入单个视频文件")
            return
        
        url = urls[0]
        file_path = url.toLocalFile()
        
        # 检查文件是否存在且是视频文件
        if not Path(file_path).exists() or not self.isVideoFile(file_path):
            QMessageBox.warning(self, "剪辑", "请拖入有效的视频文件")
            return
        
        # 显示文件路径
        self.ui.fileInfo.setText(file_path)
        
        # 获取视频信息
        self.getVideoFrameRate(file_path)
        self.getVideoDuration(file_path)
    
    def getVideoFrameRate(self, file_path):
        try:
            # 使用subprocess执行ffprobe命令
            result = subprocess.run([
                "ffprobe", 
                "-v", "error",
                "-select_streams", "v:0",
                "-show_entries", "stream=r_frame_rate",
                "-of", "default=noprint_wrappers=1:nokey=1",
                file_path
            ], capture_output=True, text=True, timeout=10)
            
            if result.returncode == 0:
                output = result.stdout.strip()
                print(f"从ffprobe返回的帧率: {output}")
                
                # 解析帧率
                if '/' in output:
                    parts = output.split('/')
                    if len(parts) == 2:
                        try:
                            numerator = float(parts[0])
                            denominator = float(parts[1])
                            if denominator != 0:
                                frame_rate = numerator / denominator
                                self.ui.video_frame_rate.setValue(frame_rate)
                                print(f"视频帧率: {frame_rate} fps")
                                return
                        except ValueError:
                            pass
                else:
                    try:
                        frame_rate = float(output)
                        if frame_rate > 0:
                            self.ui.video_frame_rate.setValue(frame_rate)
                            print(f"视频帧率: {frame_rate} fps")
                            return
                    except ValueError:
                        pass
                
                QMessageBox.warning(self, "错误", f"无法解析帧率: {output}")
            else:
                QMessageBox.warning(self, "错误", f"获取视频帧率失败:\n{result.stderr}")
                
        except subprocess.TimeoutExpired:
            QMessageBox.warning(self, "错误", "获取帧率超时")
        except Exception as e:
            QMessageBox.warning(self, "错误", f"执行ffprobe时发生错误: {str(e)}")
    
    def convertToSeconds(self, time: QTime):
        total_seconds = (time.hour() * 3600.0 + 
                        time.minute() * 60.0 + 
                        time.second() + 
                        time.msec() / 1000.0)
        print(f"convertToSeconds: {total_seconds:.3f}")
        return total_seconds
    
    def convertToTime(self, total_seconds):
        hours = int(total_seconds) // 3600
        minutes = (int(total_seconds) % 3600) // 60
        seconds = int(total_seconds) % 60
        milliseconds = int((total_seconds - int(total_seconds)) * 1000)
        
        return QTime(hours, minutes, seconds, milliseconds)
    
    def getVideoDuration(self, file_path):
        try:
            result = subprocess.run([
                "ffprobe",
                "-v", "error",
                "-show_entries", "format=duration",
                "-of", "default=noprint_wrappers=1:nokey=1",
                file_path
            ], capture_output=True, text=True, timeout=10)
            
            if result.returncode == 0:
                output = result.stdout.strip()
                try:
                    duration = float(output)
                    if duration > 0:
                        # 设置Slider最大值：帧数 = 总秒数 * 帧率
                        frame_rate = self.ui.video_frame_rate.value()
                        self.ui.horizontalSlider.setMaximum(int(duration * frame_rate))
                        print(f"视频时长: {duration} 秒")
                        return
                except ValueError:
                    pass
                
                QMessageBox.warning(self, "错误", f"无法解析视频时长: {output}")
            else:
                QMessageBox.warning(self, "错误", f"获取视频时长失败:\n{result.stderr}")
                
        except subprocess.TimeoutExpired:
            QMessageBox.warning(self, "错误", "获取时长超时")
        except Exception as e:
            QMessageBox.warning(self, "错误", f"执行ffprobe时发生错误: {str(e)}")
    
    def on_horizontalSlider_valueChanged(self, value):
        # 值变化时更新当前时间和当前帧数，但不触发刷新缩略图
        self.ui.current_frame.setValue(value)
        
        current_frame = value
        frame_rate = self.ui.video_frame_rate.value()
        current_second = current_frame / frame_rate if frame_rate > 0 else 0
        current_time = self.convertToTime(current_second)
        
        # 更新timeEdit控件显示当前时间
        self.ui.current_time.setTime(current_time)
    
    def on_horizontalSlider_sliderReleased(self):
        # 滑块释放时调用预览缩略图
        if not self.ui.fileInfo.text():
            return
        
        preview_label = self.ui.label_preview
        
        # 删除已存在的图片文件
        frame_file = "current_frame.png"
        if Path(frame_file).exists():
            try:
                Path(frame_file).unlink()
            except Exception as e:
                print(f"删除文件失败: {e}")
        
        # 获取当前时间点
        current_frame = self.ui.current_frame.value()
        frame_rate = self.ui.video_frame_rate.value()
        current_time = current_frame / frame_rate if frame_rate > 0 else 0
        current_time_str = f"{current_time:.5f}"
        
        file_path = self.ui.fileInfo.text()
        
        try:
            # 使用ffmpeg提取帧
            result = subprocess.run([
                "ffmpeg",
                "-ss", current_time_str,
                "-i", file_path,
                "-frames:v", "1",
                frame_file
            ], capture_output=True, text=True, timeout=30)
            
            if result.returncode == 0 and Path(frame_file).exists():
                print("图片生成成功!")
                
                # 加载并显示图片
                pixmap = QPixmap(frame_file)
                if not pixmap.isNull():
                    # 高质量缩放
                    scaled_pixmap = pixmap.scaled(
                        preview_label.size(),
                        Qt.KeepAspectRatio,
                        Qt.SmoothTransformation
                    )
                    preview_label.setPixmap(scaled_pixmap)
                else:
                    QMessageBox.warning(self, "错误", "加载图片失败")
            else:
                error_msg = result.stderr if result.stderr else "未知错误"
                QMessageBox.warning(self, "错误", f"生成缩略图失败:\n{error_msg}")
                
        except subprocess.TimeoutExpired:
            QMessageBox.warning(self, "错误", "生成缩略图超时")
        except Exception as e:
            QMessageBox.warning(self, "错误", f"执行ffmpeg时发生错误: {str(e)}")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    
    window = MainWindow()
    window.show()
    
    sys.exit(app.exec())