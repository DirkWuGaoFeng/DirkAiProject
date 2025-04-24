/**
 * @file stockdatamanager.cpp
 * @brief 股票数据管理类的实现文件
 * 
 * 该文件实现了StockDataManager类的所有功能，包括股票数据的获取、处理和更新。
 * 通过网络请求从新浪财经和腾讯财经获取实时和历史股票数据。
 */

#include "stockdatamanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QUrl>
#include <QStringDecoder>

/**
 * @brief 构造函数
 * @param parent 父对象指针
 * 
 * 初始化网络管理器、定时器等成员变量，并建立信号槽连接
 */
StockDataManager::StockDataManager(QObject *parent)
    : QObject(parent)
    , networkManager(new QNetworkAccessManager(this))
    , currentReply(nullptr)
    , updateTimer(new QTimer(this))
{
    setupConnections();
}

/**
 * @brief 析构函数
 * 
 * 停止数据更新并清理网络请求资源
 */
StockDataManager::~StockDataManager()
{
    stopUpdates();
    cleanupReply();
}

/**
 * @brief 设置信号槽连接
 * 
 * 建立定时器超时信号与更新数据槽函数的连接
 */
void StockDataManager::setupConnections()
{
    connect(updateTimer, &QTimer::timeout, this, &StockDataManager::updateStockData);
}

/**
 * @brief 请求股票数据
 * @param stockCode 股票代码，格式为"sh000001"或"sz000001"
 * 
 * 根据提供的股票代码请求实时和历史数据。首先验证股票代码格式，
 * 然后请求历史K线数据，并启动定时器定期更新实时数据。
 */
void StockDataManager::requestRealtimeData(const QString& stockCode)
{
    // 校验股票代码格式
    if (!stockCode.startsWith("sh") && !stockCode.startsWith("sz")) {
        emit errorOccurred("股票代码格式错误：必须以'sh'或'sz'开头");
        return;
    }
    
    QString code = stockCode.mid(2);
    if (code.length() != 6 || !code.toInt()) {
        emit errorOccurred("股票代码格式错误：必须为6位数字");
        return;
    }

    currentStockCode = stockCode;
    updateTimer->start(5000); // 每5秒更新一次实时数据
}

void StockDataManager::requestHistoricalData(const QString& stockCode, const QDateTime& startTime, const QDateTime& endTime)
{
    if (stockCode.isEmpty()) return;
    
    // 清除历史数据
    historicalData = HistoricalData();
    
    // 获取历史K线数据
    QString startTimeStr = startTime.toString("yyyy-MM-dd");
    QString endTimeStr = endTime.toString("yyyy-MM-dd");
    QNetworkRequest request(QUrl(QString("http://web.ifzq.gtimg.cn/appstock/app/fqkline/get?param=%1,day,%2,%3,100,qfq").arg(stockCode).arg(startTimeStr).arg(endTimeStr)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

    cleanupReply();
    currentReply = networkManager->get(request);
    connect(currentReply, &QNetworkReply::finished, this, &StockDataManager::onHistoricalDataReceived);
}

/**
 * @brief 停止数据更新
 * 
 * 停止定时器的自动更新，并清理当前的网络请求
 */
void StockDataManager::stopUpdates()
{
    updateTimer->stop();
    cleanupReply();
}

/**
 * @brief 清理网络请求
 * 
 * 使用互斥锁保护，安全地清理当前的网络请求对象。
 * 断开所有信号连接并释放资源。
 */
void StockDataManager::cleanupReply()
{
    QMutexLocker locker(&replyMutex);
    if (currentReply) {
        disconnect(currentReply, nullptr, this, nullptr);
        currentReply->abort();
        currentReply->deleteLater();
        currentReply = nullptr;
    }
}

/**
 * @brief 更新股票数据
 * 
 * 定时器触发时调用此函数，从新浪财经接口获取实时股票数据。
 * 如果当前没有设置股票代码，则直接返回。
 */
void StockDataManager::updateStockData()
{
    if (currentStockCode.isEmpty()) return;

    QNetworkRequest request(QUrl(QString("http://hq.sinajs.cn/list=%1").arg(currentStockCode)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    request.setRawHeader("Referer", "http://finance.sina.com.cn/");

    cleanupReply();
    currentReply = networkManager->get(request);
    connect(currentReply, &QNetworkReply::finished, this, &StockDataManager::onRealtimeDataReceived);
}

/**
 * @brief 实时数据接收完成的槽函数
 * 
 * 当网络请求完成时调用此函数，处理接收到的实时数据。
 * 使用互斥锁保护，确保线程安全。
 */
void StockDataManager::onRealtimeDataReceived()
{
    QMutexLocker locker(&replyMutex);
    if (!currentReply) return;

    if (currentReply->error() == QNetworkReply::NoError) {
        QByteArray rawData = currentReply->readAll();
        QString data = QString::fromLocal8Bit(rawData);
        processRealtimeData(data);
    } else {
        emit errorOccurred(currentReply->errorString());
    }

    currentReply->deleteLater();
    currentReply = nullptr;
}

/**
 * @brief 历史数据接收完成的槽函数
 * 
 * 当历史数据网络请求完成时调用此函数，处理接收到的K线数据。
 * 使用qobject_cast确保安全地获取发送者对象。
 */
void StockDataManager::onHistoricalDataReceived()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        processHistoricalData(data);
        emit historicalDataReceived();
    }

    else {
        emit errorOccurred(reply->errorString());
    }
    reply->deleteLater();
}

/**
 * @brief 处理实时数据
 * @param data 从新浪财经接口获取的原始数据字符串
 * 
 * 解析新浪财经接口返回的数据格式，提取所需的股票信息。
 * 数据格式示例：
 * var hq_str_sz000001="平安银行,15.640,15.630,15.640,15.680,15.510,15.630,15.640,346366700,5419491239.460,2200,15.630,15800,15.620,16300,15.610,12800,15.600,18800,15.590,2200,15.640,4300,15.650,7500,15.660,5200,15.670,6500,15.680,2023-11-24,15:00:03,00,";
 */
void StockDataManager::processRealtimeData(const QString& data)
{
    QString stockStr = data.split('"')[1];
    QStringList fields = stockStr.split(',');
    
    if (fields.size() < 32) {
        emit errorOccurred("Invalid realtime data format");
        return;
    }

    StockData stockData;
    stockData.name = fields[0].toUtf8();
    stockData.currentPrice = fields[3].toDouble();  // 当前价
    stockData.openPrice = fields[1].toDouble();     // 开盘价
    stockData.highPrice = fields[4].toDouble();     // 最高价
    stockData.lowPrice = fields[5].toDouble();      // 最低价
    stockData.closePrice = fields[2].toDouble();    // 昨收价
    stockData.timestamp = QDateTime::fromString(fields[30] + " " + fields[31], "yyyy-MM-dd hh:mm:ss");

    latestData = stockData;
    emit stockDataReceived(stockData);
}

/**
 * @brief 处理历史数据
 * @param data 从腾讯财经接口获取的JSON格式数据
 * 
 * 解析腾讯财经接口返回的JSON数据，提取历史K线信息。
 * 包括开盘价、收盘价、最高价、最低价和时间戳等信息。
 * 解析失败时会发出错误信号。
 */
void StockDataManager::processHistoricalData(const QByteArray& data)
{
    if (data.isEmpty()) {
        emit errorOccurred("Empty historical data received");
        return;
    }

    // 解析腾讯历史数据JSON格式
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit errorOccurred("Invalid historical data format");
        return;
    }

    QJsonObject root = doc.object();
    if (!root.contains("data")) {
        emit errorOccurred("JSON数据缺少data字段");
        return;
    }

    QJsonObject data_obj = root["data"].toObject();
    if (data_obj.isEmpty()) {
        emit errorOccurred("JSON数据中data字段为空");
        return;
    }

    QStringList keys = data_obj.keys();
    if (keys.isEmpty()) {
        emit errorOccurred("JSON数据中缺少股票代码字段");
        return;
    }

    QString stock_code = keys.first();
    QJsonObject stock_data = data_obj[stock_code].toObject();
    if (!stock_data.contains("qfqday")) {
        emit errorOccurred("JSON数据中缺少qfqday字段");
        return;
    }

    QJsonArray day_data = stock_data["qfqday"].toArray();
    if (day_data.isEmpty()) {
        emit errorOccurred("qfqday数据为空");
        return;
    }

    historicalData.openPrices.clear();
    historicalData.highPrices.clear();
    historicalData.lowPrices.clear();
    historicalData.closePrices.clear();
    historicalData.timestamps.clear();

    for (const QJsonValue& value : day_data) {
        // 验证日期格式
        QDateTime timestamp = QDateTime::fromString(value[0].toString(), "yyyy-MM-dd");
        if (!timestamp.isValid()) {
            emit errorOccurred("Invalid timestamp format in historical data");
            continue;
        }

        // 验证价格数据
        bool conversionOk = false;
        double openPrice = value[1].toString().toDouble(&conversionOk);
        if (!conversionOk || openPrice <= 0) {
            emit errorOccurred("Invalid open price format in historical data");
            continue;
        }

        double closePrice = value[2].toString().toDouble(&conversionOk);
        if (!conversionOk || closePrice <= 0) {
            emit errorOccurred("Invalid close price format in historical data");
            continue;
        }

        double highPrice = value[3].toString().toDouble(&conversionOk);
        if (!conversionOk || highPrice <= 0) {
            emit errorOccurred("Invalid high price format in historical data");
            continue;
        }

        double lowPrice = value[4].toString().toDouble(&conversionOk);
        if (!conversionOk || lowPrice <= 0) {
            emit errorOccurred("Invalid low price format in historical data");
            continue;
        }

        // 添加验证后的数据
        historicalData.timestamps.append(timestamp);
        historicalData.openPrices.append(openPrice);
        historicalData.closePrices.append(closePrice);
        historicalData.highPrices.append(highPrice);
        historicalData.lowPrices.append(lowPrice);
    }
    
    emit historicalDataReceived();
}

