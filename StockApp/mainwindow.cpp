#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

/**
 * @brief 主窗口构造函数
 * @param parent 父窗口指针
 * 
 * 初始化主窗口，创建并设置UI组件，初始化股票数据管理器和图表管理器。
 * 建立信号槽连接，处理股票数据的接收、历史数据的更新和错误处理。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , stockManager(new StockDataManager(this))
    , chartManager(new ChartManager(this))
{
    ui->setupUi(this);
    setupUI();

    // 连接信号和槽
    connect(stockManager, &StockDataManager::stockDataReceived,
            this, &MainWindow::onStockDataReceived);
    connect(stockManager, &StockDataManager::historicalDataReceived,
            this, &MainWindow::onHistoricalDataReceived);
    connect(stockManager, &StockDataManager::errorOccurred,
            this, &MainWindow::onErrorOccurred);
}

MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief 事件过滤器实现
 * @param obj 触发事件的对象
 * @param event 事件对象
 * @return 如果事件被处理返回true，否则返回false
 * 
 * 处理股票代码输入框的回车键事件，使用户可以通过按回车键触发搜索操作。
 * 当用户在股票代码输入框中按下回车键时，自动触发搜索功能。
 */
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == stockCodeEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            onSearchButtonClicked();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

/**
 * @brief 设置用户界面
 * 
 * 创建并初始化主窗口的用户界面组件，包括：
 * - 搜索区域：股票代码输入框和搜索按钮
 * - 图表类型选择：分时图和K线图切换
 * - 信息显示区域：股票名称和当前价格
 * - 图表显示区域：股票走势图表
 * 
 * 设置布局管理器，配置事件过滤器，建立信号槽连接。
 */
void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 创建搜索区域
    QVBoxLayout *searchContainer = new QVBoxLayout();
    QHBoxLayout *searchLayout = new QHBoxLayout();
    stockCodeEdit = new QLineEdit(this);
    stockCodeEdit->setPlaceholderText("输入股票代码（如：sh600000）");
    searchButton = new QPushButton("实时数据", this);
    searchLayout->addWidget(stockCodeEdit);
    searchLayout->addWidget(searchButton);

    // 创建历史数据查询区域
    QHBoxLayout *historyLayout = new QHBoxLayout();
    startDateEdit = new QDateTimeEdit(this);
    startDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    startDateEdit->setDateTime(QDateTime::currentDateTime().addDays(-7));
    endDateEdit = new QDateTimeEdit(this);
    endDateEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    endDateEdit->setDateTime(QDateTime::currentDateTime());
    QLabel *dateRangeLabel = new QLabel("至", this);
    dateRangeLabel->setAlignment(Qt::AlignCenter);
    historyButton = new QPushButton("历史数据", this);

    historyLayout->addWidget(startDateEdit);
    historyLayout->addWidget(dateRangeLabel);
    historyLayout->addWidget(endDateEdit);
    historyLayout->addWidget(historyButton);

    searchContainer->addLayout(searchLayout);
    searchContainer->addLayout(historyLayout);

    // 设置键盘事件处理
    stockCodeEdit->installEventFilter(this);

    // 创建图表类型选择区域
    chartTypeCombo = new QComboBox(this);
    chartTypeCombo->addItem("分时图");
    chartTypeCombo->addItem("K线图");
    searchLayout->addWidget(chartTypeCombo);

    // 创建信息显示区域
    QHBoxLayout *infoLayout = new QHBoxLayout();
    stockNameLabel = new QLabel(this);
    currentPriceLabel = new QLabel(this);
    infoLayout->addWidget(stockNameLabel);
    infoLayout->addWidget(currentPriceLabel);

    // 创建图表视图
    chartView = new QChartView(chartManager->chart(), this);
    chartView->setRenderHint(QPainter::Antialiasing);

    // 添加到主布局
    mainLayout->addLayout(searchContainer);
    mainLayout->addLayout(infoLayout);
    mainLayout->addWidget(chartView);

    // 连接信号和槽
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearchButtonClicked);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::onHistoryButtonClicked);
    connect(chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onChartTypeChanged);
}

/**
 * @brief 搜索按钮点击事件处理函数
 * 
 * 获取用户输入的股票代码，进行输入验证。
 * 如果输入有效，清除当前图表数据，并请求新的股票数据。
 * 如果输入为空，显示错误提示信息。
 */
void MainWindow::onSearchButtonClicked()
{
    QString stockCode = stockCodeEdit->text().trimmed();
    if (stockCode.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入股票代码");
        return;
    }

    // 清除图表数据
    chartManager->clearData();
    
    // 请求实时股票数据
    stockManager->requestRealtimeData(stockCode);
}

void MainWindow::onHistoryButtonClicked()
{
    QString stockCode = stockCodeEdit->text().trimmed();
    if (stockCode.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入股票代码");
        return;
    }

    QDateTime startTime = startDateEdit->dateTime();
    QDateTime endTime = endDateEdit->dateTime();

    if (startTime >= endTime) {
        QMessageBox::warning(this, "错误", "开始时间必须早于结束时间");
        return;
    }

    // 清除图表数据
    chartManager->clearData();
    
    // 请求历史股票数据
    stockManager->requestHistoricalData(stockCode, startTime, endTime);
}

/**
 * @brief 图表类型切换事件处理函数
 * @param index 选择的图表类型索引（0：分时图，1：K线图）
 * 
 * 根据用户选择的图表类型，切换显示分时图或K线图。
 * 通过ChartManager进行图表切换和数据更新。
 */
void MainWindow::onChartTypeChanged(int index)
{
    chartManager->switchChartType(index == 0 ? ChartManager::TimeSeries : ChartManager::Candlestick);
}

/**
 * @brief 股票数据接收处理函数
 * @param data 接收到的实时股票数据
 * 
 * 处理新接收到的股票数据：
 * - 更新界面上显示的股票信息
 * - 更新图表显示的实时数据
 */
void MainWindow::onStockDataReceived(const StockDataManager::StockData& data)
{
    updateStockInfo(data);
    chartManager->updateRealtimeData(data);
}

/**
 * @brief 历史数据接收处理函数
 * 
 * 获取并处理历史股票数据：
 * - 从股票数据管理器获取历史数据
 * - 更新图表显示的历史数据
 */
void MainWindow::onHistoricalDataReceived()
{
    const StockDataManager::HistoricalData& data = stockManager->getHistoricalData();
    StockDataManager::StockData stockinfo;
    stockinfo.name = data.name;
    updateStockInfo(stockinfo);
    chartManager->updateHistoricalData(data);
}

/**
 * @brief 错误处理函数
 * @param error 错误信息
 * 
 * 显示错误提示对话框，向用户展示错误信息。
 */
void MainWindow::onErrorOccurred(const QString& error)
{
    QMessageBox::warning(this, "错误", error);
}

/**
 * @brief 更新股票信息显示
 * @param data 股票数据
 * 
 * 更新界面上显示的股票信息：
 * - 更新股票名称标签
 * - 更新当前价格标签
 */
void MainWindow::updateStockInfo(const StockDataManager::StockData& data)
{
    stockNameLabel->setText(QString("股票名称: %1").arg(data.name));
    currentPriceLabel->setText(QString("当前价格: %1").arg(data.currentPrice));
}
