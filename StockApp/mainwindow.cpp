#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QListWidget>
#include <QCompleter>
#include <QStyledItemDelegate>
#include <QTextDocument>

 /**
 * @brief 自定义高亮委托类
 * 
 * 该类继承自QStyledItemDelegate，用于自定义列表项的绘制方式，
 * 支持HTML富文本渲染和选中项的高亮显示。
 * 主要用于股票代码建议列表的显示优化。
 */
class HighlightDelegate : public QStyledItemDelegate {
    public:
        /**
 * @brief 构造函数
 * @param parent 父对象指针
 */
HighlightDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
        /**
 * @brief 自定义绘制方法
 * @param painter 用于绘制的QPainter对象
 * @param option 绘制选项，包含样式信息
 * @param index 当前项的模型索引
 * 
 * 该方法使用QTextDocument渲染HTML富文本内容，
 * 并实现选中项的背景高亮效果。
 */
void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            painter->save();
            QTextDocument doc;
            doc.setHtml(index.data().toString());
            // 设置item背景色选中高亮
            if (option.state & QStyle::State_Selected) {
                painter->fillRect(option.rect, option.palette.highlight());
                doc.setDefaultStyleSheet("body {color: " + option.palette.highlightedText().color().name() + ";}");
            }
            // 调整文本位置
            QRect textRect = option.rect;
            painter->translate(textRect.topLeft());
            QRect clip(0, 0, textRect.width(), textRect.height());
            doc.setTextWidth(textRect.width());
            doc.drawContents(painter, clip);
            painter->restore();
        }
        /**
 * @brief 计算项的大小
 * @param option 样式选项
 * @param index 模型索引
 * @return 返回项的推荐大小
 * 
 * 根据HTML内容的实际大小计算项的推荐尺寸。
 */
QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
            QTextDocument doc;
            doc.setHtml(index.data().toString());
            doc.setTextWidth(option.rect.width());
            return QSize(doc.idealWidth(), doc.size().height());
        }
    };

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
    , stockCodeMap(new StockCodeMap(this))
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
    stockCodeEdit->setPlaceholderText("输入股票代码或名称（如：sh600000或平安银行）");
    searchButton = new QPushButton("实时数据", this);

    // 创建建议列表
    suggestionList = new QListWidget(this);
    // 设置自定义委托用于渲染富文本内容，支持HTML标签显示和高亮匹配文本
    suggestionList->setItemDelegate(new HighlightDelegate(suggestionList));
    //Qt::Tool 使窗口作为工具窗口，不会阻塞主窗口
    //Qt::FramelessWindowHint 无边框窗口
    //Qt::NoDropShadowWindowHint 无阴影效果
    suggestionList->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    suggestionList->setAttribute(Qt::WA_ShowWithoutActivating);
    suggestionList->setFocusPolicy(Qt::NoFocus);
    suggestionList->setMouseTracking(true);
    suggestionList->hide();

    // 创建动画效果
    QPropertyAnimation *showAnimation = new QPropertyAnimation(suggestionList, "opacity");
    showAnimation->setDuration(200);
    showAnimation->setStartValue(0);
    showAnimation->setEndValue(1);

    QPropertyAnimation *hideAnimation = new QPropertyAnimation(suggestionList, "opacity");
    hideAnimation->setDuration(200);
    hideAnimation->setStartValue(1);
    hideAnimation->setEndValue(0);

    connect(hideAnimation, &QPropertyAnimation::finished, [this]() {
        suggestionList->hide();
    });

    // 连接股票代码输入框的文本变化信号
    connect(stockCodeEdit, &QLineEdit::textChanged, this, [this, showAnimation, hideAnimation](const QString &text) {
        QString trimmedText = text.trimmed();
        if (trimmedText.isEmpty()) {
            hideAnimation->start();
            return;
        }

        // 获取匹配的股票列表
        QStringList suggestions = stockCodeMap->search(trimmedText);

        // 调试信息，检查匹配结果
        qDebug() << "输入文本: " << text << " 匹配结果: " << suggestions;

        if (suggestions.isEmpty()) {
            hideAnimation->start();
            return;
        }

        // 更新建议列表
        suggestionList->clear();
        for (const QString &suggestion : suggestions) {
            QListWidgetItem *item = new QListWidgetItem();
            QString pattern = QRegularExpression::escape(trimmedText);
            QRegularExpression regex("(" + pattern + ")", QRegularExpression::CaseInsensitiveOption);
            QString highlighted = suggestion;
            highlighted.replace(regex, "<span style='color:red;'>\\1</span>");
            item->setText(highlighted);
            suggestionList->addItem(item);
        }

        // 调整建议列表的位置和大小
        QPoint pos = stockCodeEdit->mapToGlobal(stockCodeEdit->rect().bottomLeft());
        int width = stockCodeEdit->width();
        int height = qMin(200, suggestions.count() * 25);
        suggestionList->setGeometry(pos.x(), pos.y(), width, height);

        // 确保建议列表显示在最上层
        suggestionList->raise();

        // 显示建议列表并播放动画
        suggestionList->show();
        showAnimation->start();

        // 保持输入框焦点
        stockCodeEdit->setFocus();
    });

    // 连接建议列表的项目点击信号
    connect(suggestionList, &QListWidget::itemClicked, this, [this, hideAnimation](QListWidgetItem *item) {
        QString text = item->text();
        // 使用正则表达式去除所有HTML标签，提取纯净股票代码
        QString stockCode = text;
        stockCode.remove(QRegularExpression("<[^>]*>"));
        stockCode = stockCode.split(" - ").first();
        stockCodeEdit->setText(stockCode);
        hideAnimation->start();
        stockCodeEdit->setFocus(); // 选择后重新设置焦点到输入框
    });

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

    // 创建信息显示区域和买卖盘表格的容器
    QHBoxLayout *infoOrderLayout = new QHBoxLayout();

    // 信息显示区域
    QVBoxLayout *infoLayout = new QVBoxLayout();
    stockNameLabel = new QLabel(this);
    currentPriceLabel = new QLabel(this);
    totalSharesLabel = new QLabel(this);
    marketValueLabel = new QLabel(this);
    turnoverRateLabel = new QLabel(this);
    circulatingSharesLabel = new QLabel(this);
    circulatingValueLabel = new QLabel(this);
    peRatioLabel = new QLabel(this);
    pbRatioLabel = new QLabel(this);
    infoLayout->addWidget(stockNameLabel);
    infoLayout->addWidget(currentPriceLabel);
    infoLayout->addWidget(totalSharesLabel);
    infoLayout->addWidget(marketValueLabel);
    infoLayout->addWidget(turnoverRateLabel);
    infoLayout->addWidget(circulatingSharesLabel);
    infoLayout->addWidget(circulatingValueLabel);
    infoLayout->addWidget(peRatioLabel);
    infoLayout->addWidget(pbRatioLabel);

    // 买卖盘数据表格
    QHBoxLayout *orderLayout = new QHBoxLayout();
    orderLayout->setContentsMargins(0, 0, 0, 0);

    infoOrderLayout->addLayout(infoLayout);
    infoOrderLayout->addLayout(orderLayout);

    // 创建卖盘表格
    sellOrderTable = new QTableWidget(this);
    sellOrderTable->setColumnCount(2);
    sellOrderTable->setHorizontalHeaderLabels({"卖出价", "卖出量(手)"});
    sellOrderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sellOrderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    sellOrderTable->verticalHeader()->setVisible(false);
    sellOrderTable->setMaximumWidth(200);

    // 创建买盘表格
    buyOrderTable = new QTableWidget(this);
    buyOrderTable->setColumnCount(2);
    buyOrderTable->setHorizontalHeaderLabels({"买入价", "买入量(手)"});
    buyOrderTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    buyOrderTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    buyOrderTable->verticalHeader()->setVisible(false);
    buyOrderTable->setMaximumWidth(200);

    orderLayout->addWidget(sellOrderTable);
    orderLayout->addWidget(buyOrderTable);

    // 创建图表视图
    chartView = new QChartView(chartManager->chart(), this);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(400);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 添加到主布局
    mainLayout->addLayout(searchContainer);
    mainLayout->addLayout(infoOrderLayout);
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
void MainWindow::onStockDataReceived()
{
    const StockDataManager::StockData& data = stockManager->getLatestData();
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
    const StockDataManager::StockData& stockdata = stockManager->getLatestData();
    updateStockInfo(stockdata);
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

    // 更新市场信息
    totalSharesLabel->setText(QString("总股本: %1亿").arg(data.totalShares, 0, 'f', 2));
    marketValueLabel->setText(QString("总市值: %1亿").arg(data.marketValue, 0, 'f', 2));
    turnoverRateLabel->setText(QString("换手率: %1%").arg(data.turnoverRate, 0, 'f', 2));
    circulatingSharesLabel->setText(QString("流通股: %1亿").arg(data.circulatingShares, 0, 'f', 2));
    circulatingValueLabel->setText(QString("流通值: %1亿").arg(data.circulatingValue, 0, 'f', 2));
    peRatioLabel->setText(QString("市盈率: %1").arg(data.peRatio, 0, 'f', 2));
    pbRatioLabel->setText(QString("市净率: %1").arg(data.pbRatio, 0, 'f', 2));

    // 更新买卖盘数据
    buyOrderTable->setRowCount(5);
    sellOrderTable->setRowCount(5);
    for (int i = 0; i < 5; i++) {
        // 买盘数据
        QTableWidgetItem *buyPriceItem = new QTableWidgetItem(QString::number(data.buyPrices[i], 'f', 2));
        QTableWidgetItem *buyVolumeItem = new QTableWidgetItem(QString::number(data.buyVolumes[i] / 100, 'f', 0));
        buyOrderTable->setItem(i, 0, buyPriceItem);
        buyOrderTable->setItem(i, 1, buyVolumeItem);

        // 卖盘数据
        QTableWidgetItem *sellPriceItem = new QTableWidgetItem(QString::number(data.sellPrices[i], 'f', 2));
        QTableWidgetItem *sellVolumeItem = new QTableWidgetItem(QString::number(data.sellVolumes[i] / 100, 'f', 0));
        sellOrderTable->setItem(i, 0, sellPriceItem);
        sellOrderTable->setItem(i, 1, sellVolumeItem);
    }
}
