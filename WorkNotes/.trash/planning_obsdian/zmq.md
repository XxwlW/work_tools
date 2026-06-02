# tda4_recorder_qt

recorder.pro
main.cpp
mainwindow.h / mainwindow.cpp / mainwindow.ui
zmqsubscriber.h / zmqsubscriber.cpp
filewriter.h / filewriter.cpp
config.xml (可选)

```
构建与运行步骤（简洁）

用 vcpkg 或其他方式在 Windows 安装 libzmq + cppzmq，确保 .lib/.dll 与头文件可用。
在 Qt Creator 中打开 recorder.pro，调整 INCLUDEPATH / LIBS 为你的 zmq 路径。
编译并运行；在 UI 输入 TDA4 IP、port（示例使用 5556），点击 Start 开始录制，数据将按单条文件保存在程序目录下的 record 子目录。
兼容性与注意事项

若需要与原 plot_viewer 的文件格式 100% 一致，先确认 plot_viewer 的录制格式（是否有前缀、时间戳、打包策略）。如果 plot_viewer 只是直接把 boost 序列化 buffer 写盘，直接写 msg.data() 即可。
为防止丢包：增大 ZMQ HWM、使用内存队列并异步写盘；测试网络抖动情况下是否稳定。
可选添加：写入长度前缀、生成 meta.json、上传支持与文件滚动策略。
若需要远程触发 TDA4 上的 plot_viewer 设置（如修改 config.xml），可用 WinSCP/SSH 工具（另写模块）。
结论

完全可以用 Qt5.11.3 实现 Windows recorder。以上示例给出最小可行实现：ZMQ subscriber 在独立线程接收，把 byte[] 发到写盘线程写文件。若需要，我可给出完整 Qt Creator 工程骨架与更完善的滚动/元数据实现。
```



## recorder.pro

```
QT += core gui widgets
CONFIG += c++11
SOURCES += main.cpp mainwindow.cpp zmqsubscriber.cpp filewriter.cpp
HEADERS += mainwindow.h zmqsubscriber.h filewriter.h
FORMS += mainwindow.ui
# adjust include/link paths for zmq on your machine
INCLUDEPATH += "C:/vcpkg/installed/x64-windows/include"
LIBS += -L"C:/vcpkg/installed/x64-windows/lib" -lzmq
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
#include <QMainWindow>
#include <QThread>
#include "zmqsubscriber.h"
#include "filewriter.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent=nullptr);
    ~MainWindow();
private:
    ZmqSubscriber* subscriber_;
    QThread* subThread_;
    FileWriter* writer_;
private slots:
    void onStart();
    void onStop();
    void onMessageReceived(const QByteArray& data);
};
```

## mainwindow.cpp 

```
#include "mainwindow.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QDir>

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent) {
    auto *btnStart = new QPushButton("Start", this);
    auto *btnStop = new QPushButton("Stop", this);
    auto *lay = new QVBoxLayout;
    lay->addWidget(btnStart); lay->addWidget(btnStop);
    auto *w = new QWidget; w->setLayout(lay); setCentralWidget(w);

    // writer runs in main thread (or can be moved to its own thread)
    writer_ = new FileWriter(QDir::currentPath() + "/record");
    connect(btnStart, &QPushButton::clicked, this, &MainWindow::onStart);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::onStop);
}

MainWindow::~MainWindow(){
    onStop();
}

void MainWindow::onStart(){
    if (subscriber_) return;
    subThread_ = new QThread;
    subscriber_ = new ZmqSubscriber("tcp://10.18.19.251:5556"); // 从 config 读取
    subscriber_->moveToThread(subThread_);
    connect(subThread_, &QThread::started, subscriber_, &ZmqSubscriber::start);
    connect(subThread_, &QThread::finished, subscriber_, &ZmqSubscriber::stop);
    connect(subscriber_, &ZmqSubscriber::messageReceived, this, &MainWindow::onMessageReceived);
    subThread_->start();
}

void MainWindow::onStop(){
    if (!subscriber_) return;
    subThread_->quit();
    subThread_->wait();
    delete subscriber_; subscriber_ = nullptr;
    delete subThread_; subThread_ = nullptr;
}

void MainWindow::onMessageReceived(const QByteArray& data){
    writer_->enqueue(data);
}
```

 mainwindow.ui

```

```

## zmqsubscriber.h

```
#pragma once
#include <QObject>
#include <QAtomicBool>
#include <QByteArray>
#include <string>

class ZmqSubscriber : public QObject {
    Q_OBJECT
public:
    explicit ZmqSubscriber(const std::string& endpoint, QObject* parent=nullptr);
    ~ZmqSubscriber();
public slots:
    void start();
    void stop();
signals:
    void messageReceived(const QByteArray& data);
private:
    std::string endpoint_;
    QAtomicBool running_;
};

```



## zmqsubscriber.cpp

```
#include "zmqsubscriber.h"
#include <zmq.hpp> // cppzmq header
#include <chrono>
#include <thread>

ZmqSubscriber::ZmqSubscriber(const std::string& endpoint, QObject* parent)
    : QObject(parent), endpoint_(endpoint), running_(false) {}

ZmqSubscriber::~ZmqSubscriber(){}

void ZmqSubscriber::start(){
    running_ = true;
    try {
        zmq::context_t ctx(1);
        zmq::socket_t sub(ctx, ZMQ_SUB);
        sub.setsockopt(ZMQ_RCVHWM, 10000);
        sub.connect(endpoint_);
        sub.setsockopt(ZMQ_SUBSCRIBE, "", 0); // 订阅所有 topic
        while (running_) {
            zmq::message_t msg;
            bool ok = sub.recv(&msg, zmq::recv_flags::none);
            if (!ok) continue;
            QByteArray data((const char*)msg.data(), msg.size());
            emit messageReceived(data);
        }
        sub.close();
    } catch (const std::exception& e) {
        // 可用 signal 通知 UI
    }
}

void ZmqSubscriber::stop(){
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
#include <QFile>
#include <QThread>

class FileWriter : public QObject {
    Q_OBJECT
public:
    explicit FileWriter(const QString& dir, QObject* parent=nullptr);
    ~FileWriter();
    void enqueue(const QByteArray& data);
private:
    void runWriter();
    QString outDir_;
    QQueue<QByteArray> queue_;
    QMutex mu_;
    QWaitCondition cond_;
    bool stop_;
    QFile currentFile_;
    QThread writerThread_;
private slots:
    void onThreadStarted();
    void onThreadStopped();
};

```

## filewriter.cpp

```
#include "filewriter.h"
#include <QDir>
#include <QDateTime>

FileWriter::FileWriter(const QString& dir, QObject* parent)
    : QObject(parent), outDir_(dir), stop_(false) {
    QDir d; d.mkpath(outDir_);
    // 移到独立线程
    this->moveToThread(&writerThread_);
    connect(&writerThread_, &QThread::started, this, &FileWriter::onThreadStarted);
    writerThread_.start();
}

FileWriter::~FileWriter(){
    stop_ = true;
    cond_.wakeAll();
    writerThread_.quit();
    writerThread_.wait();
}

void FileWriter::enqueue(const QByteArray& data){
    QMutexLocker lk(&mu_);
    queue_.enqueue(data);
    cond_.wakeOne();
}

void FileWriter::onThreadStarted(){
    while (!stop_){
        QByteArray item;
        {
            QMutexLocker lk(&mu_);
            if (queue_.isEmpty()) cond_.wait(&mu_);
            if (stop_) break;
            if (!queue_.isEmpty()) item = queue_.dequeue();
        }
        if (!item.isEmpty()){
            // 每条消息另存为单文件（可改为追加写入滚动文件）
            QString fname = outDir_ + "/" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss_zzz") + ".bin";
            QFile f(fname);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(item);
                f.close();
            }
        }
    }
}
```



