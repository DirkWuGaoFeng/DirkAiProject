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
 * @param stockCode 股票代码，格式为"sh000001"或"sz000001"或"bj000001"
 * 
 * 根据提供的股票代码请求实时和历史数据。首先验证股票代码格式，
 * 然后请求历史K线数据，并启动定时器定期更新实时数据。
 */
void StockDataManager::requestRealtimeData(const QString& stockCode)
{
    // 校验股票代码格式
    if (!stockCode.startsWith("sh") && !stockCode.startsWith("sz") && !stockCode.startsWith("bj")) {
        emit errorOccurred("股票代码格式错误：必须以'sh'或'sz'或'bj'开头");
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
    
    getStockClosingInfo(stockCode);

    // 清除历史数据
    historicalData = HistoricalData();
    
    // 获取历史K线数据
    QString startTimeStr = startTime.toString("yyyy-MM-dd");
    QString endTimeStr = endTime.toString("yyyy-MM-dd");
    QNetworkRequest request(QUrl(QString("http://web.ifzq.gtimg.cn/appstock/app/fqkline/get?param=%1,day,%2,%3,100,qfq").arg(stockCode).arg(startTimeStr).arg(endTimeStr)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

    stopUpdates();
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

void StockDataManager::getStockClosingInfo(const QString& stockCode)
{
    QNetworkRequest request(QUrl(QString("http://qt.gtimg.cn/q=%1").arg(stockCode)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

    QNetworkReply *reply  = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray rawData = reply->readAll();
            QString data = QString::fromLocal8Bit(rawData);
            processRealtimeData(data);
        } else {
            emit errorOccurred(reply->errorString());
        }

        reply->deleteLater();
    });
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

    QNetworkRequest request(QUrl(QString("http://qt.gtimg.cn/q=%1").arg(currentStockCode)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0");

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
        emit stockDataReceived();
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
    if (!currentReply) return;

    if (currentReply->error() == QNetworkReply::NoError) {
        QByteArray data = currentReply->readAll();
        processHistoricalData(data);
        emit historicalDataReceived();
    }

    else {
        emit errorOccurred(currentReply->errorString());
    }
    currentReply->deleteLater();
    currentReply = nullptr;
}

/**
 * @brief 处理实时数据
 * @param data 从新浪财经接口获取的原始数据字符串
 * 
 * 解析新浪财经接口返回的数据格式，提取所需的股票信息。
 * 数据格式示例：
 * var v_sz000858="51~五 粮 液~000858~129.68~129.06~129.20~54265~27537~26711~129.67~12~129.65~5~129.64~2~129.63~7~129.62~2~129.68~88~129.69~17~129.70~26~129.71~15~129.72~7~~20250425111454~0.62~0.48~130.22~128.96~129.68/54265/703736059~54265~70374~0.14~15.58~~130.22~128.96~0.98~5033.56~5033.67~3.98~141.97~116.15~1.07~-125~129.69~15.14~16.66~~~1.22~70373.6059~0.0000~0~ ~GP-A~-5.66~-1.57~5.59~23.69~19.58~176.18~103.75~-1.46~-3.15~1.49~3881525907~3881608005~-69.06~4.90~3881525907~~~-8.50~0.14~~CNY~0~~129.77~-45";
 */
void StockDataManager::processRealtimeData(const QString& data)
{
    QString stockStr = data.split('"')[1];
    QStringList fields = stockStr.split('~');
    
    if (fields.size() < 32) {
        emit errorOccurred("Invalid realtime data format");
        return;
    }

    latestData.name = fields[1].toUtf8();
    latestData.currentPrice = fields[3].toDouble();  // 当前价
    latestData.openPrice = fields[5].toDouble();     // 开盘价
    latestData.highPrice = fields[33].toDouble();     // 最高价
    latestData.lowPrice = fields[34].toDouble();      // 最低价
    latestData.closePrice = fields[4].toDouble();    // 昨收价
    latestData.timestamp = QDateTime::fromString(fields[30], "yyyyMMddhhmmss");
    
    // 解析买卖盘数据
    for(int i = 0; i < 5; i++) {
        latestData.buyVolumes[i] = fields[10 + i*2].toDouble();
        latestData.buyPrices[i] = fields[9 + i*2].toDouble();
        latestData.sellVolumes[i] = fields[20 + i*2].toDouble();
        latestData.sellPrices[i] = fields[19 + i*2].toDouble();
    }
    
    // 计算其他指标
    double volume = fields[36].toDouble();  // 成交量
    double amount = fields[37].toDouble();  // 成交额
    latestData.marketValue = fields[45].toDouble();  // 总市值
    latestData.totalShares = latestData.marketValue / latestData.currentPrice;  // 总股本（单位：股）
    latestData.circulatingValue = fields[44].toDouble();  // 流通市值
    latestData.circulatingShares = latestData.circulatingValue / latestData.totalShares;  // 流通股本
    latestData.turnoverRate = fields[38].toDouble();  // 换手率
    latestData.peRatio = fields[39].toDouble();  // 市盈率
    latestData.pbRatio = fields[46].toDouble();  // 市净率
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

    if (!stock_data.contains("qt")) {
        emit errorOccurred("JSON数据中缺少qt字段");
        return;
    }

    QJsonObject qt_data = stock_data["qt"].toObject();
    if (qt_data.isEmpty()) {
        emit errorOccurred("qt数据为空");
        return;
    }

    historicalData.name = qt_data["zjlx"][12].toString().toUtf8();
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
}

