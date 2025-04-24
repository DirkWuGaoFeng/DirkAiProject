#ifndef STOCKDATAMANAGER_H
#define STOCKDATAMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMutex>
#include <QTimer>
#include <QDateTime>
#include <QVector>

/**
 * @brief 股票数据管理类
 * 
 * StockDataManager类负责管理股票数据的获取、处理和更新。
 * 该类提供实时和历史股票数据的请求和处理功能，并通过信号机制通知数据更新。
 */
class StockDataManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 实时股票数据结构
     */
    struct StockData {
        QString name;
        double currentPrice;
        double openPrice;
        double highPrice;
        double lowPrice;
        double closePrice;
        QDateTime timestamp;
    };

    /**
     * @brief 历史股票数据结构
     */
    struct HistoricalData {
        QVector<double> openPrices;
        QVector<double> highPrices;
        QVector<double> lowPrices;
        QVector<double> closePrices;
        QVector<QDateTime> timestamps;
    };

    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit StockDataManager(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~StockDataManager();

    /**
     * @brief 请求实时股票数据
     * @param stockCode 股票代码
     * 
     * 根据提供的股票代码请求实时数据
     */
    void requestRealtimeData(const QString& stockCode);

    /**
     * @brief 请求历史股票数据
     * @param stockCode 股票代码
     * @param startTime 开始时间
     * @param endTime 结束时间
     * 
     * 根据提供的股票代码和时间范围请求历史数据
     */
    void requestHistoricalData(const QString& stockCode, const QDateTime& startTime, const QDateTime& endTime);

    /**
     * @brief 停止数据更新
     * 
     * 停止定时器并清理网络请求
     */
    void stopUpdates();

    /**
     * @brief 获取历史数据
     * @return 返回历史数据的常量引用
     */
    const HistoricalData& getHistoricalData() const { return historicalData; }

    /**
     * @brief 检查是否正在更新数据
     * @return 如果正在更新返回true，否则返回false
     */
    bool isUpdating() const { return updateTimer->isActive(); }

signals:
    /**
     * @brief 股票数据接收信号
     * @param data 接收到的实时股票数据
     */
    void stockDataReceived(const StockData& data);

    /**
     * @brief 历史数据接收完成信号
     */
    void historicalDataReceived();

    /**
     * @brief 错误发生信号
     * @param error 错误信息
     */
    void errorOccurred(const QString& error);

private slots:
    /**
     * @brief 实时数据接收槽函数
     */
    void onRealtimeDataReceived();

    /**
     * @brief 历史数据接收槽函数
     */
    void onHistoricalDataReceived();

    /**
     * @brief 更新股票数据槽函数
     */
    void updateStockData();

private:
    /**
     * @brief 处理实时数据
     * @param data 接收到的实时数据字符串
     */
    void processRealtimeData(const QString& data);

    /**
     * @brief 处理历史数据
     * @param data 接收到的历史数据字节数组
     */
    void processHistoricalData(const QByteArray& data);

    /**
     * @brief 设置信号槽连接
     */
    void setupConnections();

    /**
     * @brief 清理网络请求
     */
    void cleanupReply();

    QNetworkAccessManager* networkManager;  ///< 网络访问管理器
    QNetworkReply* currentReply;          ///< 当前网络请求
    QMutex replyMutex;                    ///< 请求互斥锁
    QTimer* updateTimer;                   ///< 更新定时器
    QString currentStockCode;              ///< 当前股票代码
    HistoricalData historicalData;         ///< 历史数据
    StockData latestData;                  ///< 最新数据
};

#endif // STOCKDATAMANAGER_H