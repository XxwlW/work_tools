# tda4_recorder_qt

```
TDA4 Recorder (Qt)

说明:
- 基于 Qt5.11.3 + cppzmq(libzmq) 的简单 recorder。
- 程序订阅 tcp://IP:PORT（默认 UI 提供输入），将接收到的每条 ZMQ message 原样保存为单独二进制文件（.bin）。
- 若需与 Linux plot_viewer 录制文件 100% 一致，请确认 plot_viewer 是否对数据做了额外封装（时间戳/长度前缀）。本程序直接写 payload。

构建:
1. 安装 Qt 5.11.3（Qt Creator 推荐）。
2. 使用 vcpkg 或其他方式安装 libzmq + cppzmq。
   示例: vcpkg install zeromq cppzmq
3. 修改 recorder.pro 中 INCLUDEPATH / LIBS 指向你的 libzmq 路径。
4. 在 Qt Creator 中打开 recorder.pro，构建并运行。

使用:
- 填写设备 IP 和 Port（与 TDA4 的 publisher 匹配）。
- 选择保存目录，点击「开始」后开始接收并保存。
- 点击「停止」停止订阅并退出线程。

注意:
- 网络不稳定时请增大 ZMQ HWM 或在写盘层增加更强的队列/持久化机制。
- 可扩展: 将多条消息打包成滚动文件、添加长度前缀或生成 metadata.json。

```

## readme

下面用中文简洁地、按模块说明如何实现并验证 tda4_recorder_qt 的具体内容、工作流程与与现有 Linux 脚本/plot_viewer 的兼容性要点。

1. 总体目标（一句话）
   1. 在 Windows 上用 Qt 客户端替代 Linux 上的 plot_viewer 录制功能：直接订阅 TDA4 发布的 ZMQ 数据流，将接收到的 payload 按原始二进制格式保存到磁盘，不改动项目源码。
2. 与原脚本 record_planning_data.sh 的关系
   1. 原脚本作用：修改 conf/tools/plot_viewer/config.xml（设置 publisher IP、记录目录、开启录制选项），杀掉旧 plot_viewer 进程，source 环境并启动 bin_x86/plot_viewer（在 Linux 上运行）。
   2. Windows recorder 不执行这些操作，也不改变项目源码：它直接根据 UI 填写的 TDA4 IP/Port 连接到发布端订阅数据并保存，因此不需要 SSH 到 TDA4、也不需要启动 plot_viewer。
3. 关键模块及职责
   1. UI (MainWindow)
      1. 输入：TDA4 IP、Port、保存目录、数据保存策略（单文件/滚动/每条单文件）。
      2. 控制：Start/Stop、状态与最近消息信息显示。
   2. ZmqSubscriber (QThread)
      1. 使用 libzmq/cppzmq 在独立线程里建立 ZMQ SUB socket，设置 HWM，connect(tcp://IP:PORT)，subscribe all 或指定 topic。
      2. 接收时把每条消息（或多帧）转换为 QByteArray 并通过 signal 发送给写盘模块。
      3. 提供错误 signal（连接失败、异常）。
   3. FileWriter (Worker thread)
      1. 异步接收数据队列（BlockingQueue 风格），把 payload 原样写入磁盘。
      2. 写盘策略可选：
         1. 每条消息单独文件 (.bin)：兼容性最直接，便于单条回放。
         2. 追加写入滚动文件：在每条消息前写入固定长度前缀（4/8 字节的 uint32/uint64 表示该消息长度），方便后续批量解析（推荐用于效率）。
         3. 确保线程安全、定期 flush、文件轮换、磁盘空间监控。
   4. Config（可选）
      1. 可读取项目 config.xml 以获取默认 IP/端口与选项，但不修改项目文件（或提供“下载/编辑并上传”功能，需 SSH）。
4. 数据格式兼容性（最关键）
   1. 目标：保存文件与 Linux plot_viewer 录制的文件能够被后续回放/重注入工具识别。
   2. 首先确认 plot_viewer 在 Linux 里如何写盘：
      1. 打开 tools/plot_viewer 源码或查看已有录制文件（data_from_tda4 下的文件），检查是否每条消息为单文件，还是连续文件带长度前缀或带时间戳头。
   3. 两种常见兼容策略：
      1. 如果 plot_viewer 写入的是纯 payload（boost 序列化后的二进制 buffer）且每条写为独立文件：Windows recorder 直接把 msg.data() 写为单文件即可 100% 兼容。
      2. 如果 plot_viewer 将多条消息追加到一个文件并写入长度前缀或元信息：Windows recorder 需复制相同的封装（例如在写盘前写入 4 字节 len，然后写 payload），以便文件格式一致。
   4. Multipart ZMQ：若 publisher 发送 multipart messages，必须保留帧边界或合并帧为同一 payload，按 plot_viewer 的处理方式保存（检查 plot_viewer 接收实现）。
5. 接收/写盘细节与鲁棒性
   1. ZMQ 参数：设置 ZMQ_RCVHWM (如 10000)、合理超时与重连策略。
   2. 线程模型：接收线程快速入队 -> 写盘线程处理 I/O，避免接收被磁盘阻塞。
   3. 写盘细节：避免频繁打开/关闭导致性能问题；可缓存并周期性 flush；停止时强制 flush 并完整关闭文件。
   4. 日志与 UI 提示：记录接收速率、错误、写盘失败、磁盘剩余空间。
   5. 优雅退出：Stop/Close 时等待队列写空再退出，确保数据无损。
6. 构建/依赖（简明）
   1. Qt 5.11.3 (Qt Creator)。
   2. libzmq + cppzmq（windows）：建议用 vcpkg 安装：vcpkg install zeromq cppzmq，然后在 .pro 中设置 INCLUDEPATH/ LIBS 指向 vcpkg 的路径。
   3. 针对 MSVC/MinGW 链接 winsock（-lws2_32）等（recorder.pro 中已示例）。
   4. 打包运行需把 libzmq 的 DLL 和 Qt 运行时一并部署。
7. 运行与验证流程
   1. 步骤：
      1. 在 TDA4 上（或使用原 plot_viewer）确认 publisher 的 IP/PORT 与 topic（或查看 conf/tools/plot_viewer/config.xml）。
      2. 在 Windows recorder UI 填写 IP/PORT、设置保存目录，Start。
      3. 在 TDA4 端产生数据（运行规划模块），Windows recorder 应显示接收计数并在目录生成 .bin 文件或滚动文件。
      4. 验证兼容性：用原工具（plot_viewer 或回放脚本）读取或反序列化 Windows 生成的文件，或对比 Linux 录制的样本文件内容/长度前缀确认相同。
      5. 若不能直接回放，检查是否缺少前缀/元数据或帧边界不一致，按需调整写盘封装。
8. 常见问题与排查建议
   1. 无数据接收：检查 TDA4 是否有发布、端口是否正确、防火墙/路由是否阻止（Windows 防火墙、网卡设置）。
   2. 数据不完整/丢帧：增大 ZMQ HWM、确保写盘速度足够、使用更大的内存队列或使用 SSD。
   3. 格式不兼容：对比文件头十六进制内容，确认是否有额外时间戳/长度前缀；查看 Linux plot_viewer 的记录代码确认写盘封装。
   4. 多帧消息：检查 publisher 是否发送 multipart；若是，应把所有 frames 合并或保存成一组文件并记录边界元信息。
9. 建议的最小兼容实现（总结）
   1. 默认实现：每条 ZMQ message -> 单独 .bin 文件，内容为 msg.data()（最简单、最保险）。
   2. 可选增强：支持“长度前缀追加文件”模式（在配置中切换），以匹配 plot_viewer 的连续文件格式。
   3. 提供“读取并验证”功能：用 proto 或 boost 反序列化部分样本，验证数据能否被后端工具识别。



## recorder.pro

```
QT += core gui widgets
CONFIG += c++11
TARGET = tda4_recorder_qt
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/zmqsubscriber.cpp \
    src/filewriter.cpp

HEADERS += \
    src/mainwindow.h \
    src/zmqsubscriber.h \
    src/filewriter.h

FORMS += \
    src/mainwindow.ui

# 请根据本机调整以下路径（示例使用 vcpkg 默认安装位置）
INCLUDEPATH += "C:/vcpkg/installed/x64-windows/include"
LIBS += -L"C:/vcpkg/installed/x64-windows/lib" -lzmq

# Windows: 若需要，添加 ws2_32 (Winsock)
win32:LIBS += -lws2_32
```

## main.cpp

```
#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
```

## mainwindow.h

```
#pragma once
#include <QMainWindow>
#include <QThread>
#include <memory>
#include "zmqsubscriber.h"
#include "filewriter.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private slots:
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_browseButton_clicked();
    void handleMessage(const QByteArray &data);
private:
    Ui::MainWindow *ui;
    ZmqSubscriber *subscriber_ = nullptr;
    QThread *subThread_ = nullptr;
    FileWriter *writer_ = nullptr;
    void cleanupSubscriber();
};
```

## mainwindow.cpp 

```
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->ipLineEdit->setText("10.18.19.251");
    ui->portLineEdit->setText("5556");
    ui->outDirLineEdit->setText(QDir::currentPath() + "/record");
    writer_ = new FileWriter(ui->outDirLineEdit->text(), this);
}

MainWindow::~MainWindow()
{
    on_stopButton_clicked();
    delete ui;
}

void MainWindow::on_browseButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("选择保存目录"),
                                                    ui->outDirLineEdit->text());
    if (!dir.isEmpty()) ui->outDirLineEdit->setText(dir);
    if (writer_) writer_->setOutputDir(ui->outDirLineEdit->text());
}

void MainWindow::on_startButton_clicked()
{
    if (subscriber_) {
        QMessageBox::information(this, tr("提示"), tr("已在录制"));
        return;
    }

    QString ip = ui->ipLineEdit->text().trimmed();
    QString port = ui->portLineEdit->text().trimmed();
    if (ip.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("请填写 IP 和 Port"));
        return;
    }

    QString endpoint = QString("tcp://%1:%2").arg(ip, port);
    subThread_ = new QThread(this);
    subscriber_ = new ZmqSubscriber(endpoint.toStdString());
    subscriber_->moveToThread(subThread_);
    connect(subThread_, &QThread::started, subscriber_, &ZmqSubscriber::start);
    connect(subThread_, &QThread::finished, subscriber_, &ZmqSubscriber::stop);
    connect(subscriber_, &ZmqSubscriber::messageReceived, this, &MainWindow::handleMessage);
    connect(subscriber_, &ZmqSubscriber::errorOccured, this, [this](const QString &err){
        ui->statusLabel->setText(err);
    });

    subThread_->start();
    ui->statusLabel->setText("正在订阅: " + endpoint);
}

void MainWindow::on_stopButton_clicked()
{
    cleanupSubscriber();
    ui->statusLabel->setText("已停止");
}

void MainWindow::cleanupSubscriber()
{
    if (!subscriber_) return;
    subThread_->quit();
    subThread_->wait();
    delete subscriber_;
    subscriber_ = nullptr;
    delete subThread_;
    subThread_ = nullptr;
}

void MainWindow::handleMessage(const QByteArray &data)
{
    if (writer_) {
        writer_->enqueue(data);
        ui->lastMessageLabel->setText(QString("接收 %1 bytes").arg(data.size()));
    }
}
```

## mainwindow.ui



## zmqsubscriber.h

```
#pragma once
#include <QObject>
#include <QAtomicBool>
#include <QByteArray>
#include <string>

class ZmqSubscriber : public QObject
{
    Q_OBJECT
public:
    explicit ZmqSubscriber(const std::string &endpoint, QObject *parent = nullptr);
    ~ZmqSubscriber() override;
public slots:
    void start();
    void stop();
signals:
    void messageReceived(const QByteArray &data);
    void errorOccured(const QString &err);
private:
    std::string endpoint_;
    QAtomicBool running_;
};
```

## zmqsubscriber.cpp

```
#include "zmqsubscriber.h"
#include <zmq.hpp>
#include <exception>

ZmqSubscriber::ZmqSubscriber(const std::string &endpoint, QObject *parent)
    : QObject(parent), endpoint_(endpoint), running_(false)
{}

ZmqSubscriber::~ZmqSubscriber()
{
    stop();
}

void ZmqSubscriber::start()
{
    running_ = true;
    try {
        zmq::context_t ctx(1);
        zmq::socket_t sub(ctx, ZMQ_SUB);
        int hwm = 10000;
        sub.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
        sub.connect(endpoint_);
        // 订阅全部主题
        sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);

        while (running_) {
            zmq::message_t msg;
            // blocking recv
            bool ok = sub.recv(&msg, zmq::recv_flags::none);
            if (!ok) continue;
            QByteArray data(reinterpret_cast<const char*>(msg.data()), static_cast<int>(msg.size()));
            emit messageReceived(data);
        }
        sub.close();
    } catch (const zmq::error_t &e) {
        emit errorOccured(QString("ZMQ 错误: %1").arg(e.what()));
    } catch (const std::exception &e) {
        emit errorOccured(QString("异常: %1").arg(e.what()));
    }
}

void ZmqSubscriber::stop()
{
    running_ = false;
}
```

## filewriter.h

```
#pragma once
#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <QDir>

class FileWriter : public QObject
{
    Q_OBJECT
public:
    explicit FileWriter(const QString &outDir, QObject *parent = nullptr);
    ~FileWriter() override;
    void enqueue(const QByteArray &data);
    void setOutputDir(const QString &outDir);
private:
    void run();
    QString outDir_;
    QQueue<QByteArray> queue_;
    QMutex mu_;
    QWaitCondition cond_;
    bool stop_;
    QThread workerThread_;
private slots:
    void onThreadStarted();
};
```

## filewriter.cpp

```
#include "filewriter.h"
#include <QDateTime>
#include <QFile>
#include <QCoreApplication>

FileWriter::FileWriter(const QString &outDir, QObject *parent)
    : QObject(parent), outDir_(outDir), stop_(false)
{
    QDir().mkpath(outDir_);
    moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::started, this, &FileWriter::onThreadStarted);
    workerThread_.start();
}

FileWriter::~FileWriter()
{
    {
        QMutexLocker lk(&mu_);
        stop_ = true;
        cond_.wakeAll();
    }
    workerThread_.quit();
    workerThread_.wait();
}

void FileWriter::setOutputDir(const QString &outDir)
{
    QMutexLocker lk(&mu_);
    outDir_ = outDir;
    QDir().mkpath(outDir_);
}

void FileWriter::enqueue(const QByteArray &data)
{
    QMutexLocker lk(&mu_);
    queue_.enqueue(data);
    cond_.wakeOne();
}

void FileWriter::onThreadStarted()
{
    run();
}

void FileWriter::run()
{
    while (true) {
        QByteArray item;
        {
            QMutexLocker lk(&mu_);
            if (queue_.isEmpty() && !stop_) {
                cond_.wait(&mu_);
            }
            if (stop_ && queue_.isEmpty()) break;
            if (!queue_.isEmpty()) item = queue_.dequeue();
        }
        if (!item.isEmpty()) {
            // 每条消息写为单独文件，文件名包含时间戳和微秒序号
            QString fname = outDir_ + "/" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss_zzz");
            static quint64 seq = 0;
            seq++;
            fname += QString("_%1.bin").arg(seq, 6, 10, QChar('0'));
            QFile f(fname);
            if (f.open(QIODevice::WriteOnly)) {
                // 直接写入原始 payload，保持与 plot_viewer 的兼容性
                f.write(item);
                f.close();
            }
        }
        QCoreApplication::processEvents(); // 让 Qt 事件循环响应（可选）
    }
}
```

## config.xml (可选)

```
<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>MainWindow</class>
 <widget class="QMainWindow" name="MainWindow">
  <property name="geometry">
   <rect>
    <x>0</x><y>0</y><width>480</width><height>220</height>
   </rect>
  </property>
  <property name="windowTitle">
   <string>TDA4 Recorder (Qt)</string>
  </property>
  <widget class="QWidget" name="centralwidget">
   <layout class="QVBoxLayout" name="verticalLayout">
    <item>
     <layout class="QGridLayout" name="gridLayout">
      <item row="0" column="0"><widget class="QLabel" name="label_ip"><property name="text"><string>设备 IP：</string></property></widget></item>
      <item row="0" column="1"><widget class="QLineEdit" name="ipLineEdit"/></item>
      <item row="1" column="0"><widget class="QLabel" name="label_port"><property name="text"><string>端口：</string></property></widget></item>
      <item row="1" column="1"><widget class="QLineEdit" name="portLineEdit"/></item>
      <item row="2" column="0"><widget class="QLabel" name="label_out"><property name="text"><string>保存目录：</string></property></widget></item>
      <item row="2" column="1"><widget class="QLineEdit" name="outDirLineEdit"/></item>
      <item row="2" column="2"><widget class="QPushButton" name="browseButton"><property name="text"><string>浏览</string></property></widget></item>
     </layout>
    </item>
    <item>
     <layout class="QHBoxLayout" name="hLayoutButtons">
      <item><widget class="QPushButton" name="startButton"><property name="text"><string>开始</string></property></widget></item>
      <item><widget class="QPushButton" name="stopButton"><property name="text"><string>停止</string></property></widget></item>
     </layout>
    </item>
    <item><widget class="QLabel" name="statusLabel"><property name="text"><string>状态: 空闲</string></property></widget></item>
    <item><widget class="QLabel" name="lastMessageLabel"><property name="text"><string>最近消息: 无</string></property></widget></item>
   </layout>
  </widget>
 </widget>
 <resources/>
 <connections>
  <connection>
   <sender>browseButton</sender>
   <signal>clicked()</signal>
   <receiver>MainWindow</receiver>
   <slot>on_browseButton_clicked()</slot>
  </connection>
  <connection>
   <sender>startButton</sender>
   <signal>clicked()</signal>
   <receiver>MainWindow</receiver>
   <slot>on_startButton_clicked()</slot>
  </connection>
  <connection>
   <sender>stopButton</sender>
   <signal>clicked()</signal>
   <receiver>MainWindow</receiver>
   <slot>on_stopButton_clicked()</slot>
  </connection>
 </connections>
</ui>
```

