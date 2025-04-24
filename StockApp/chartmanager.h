#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QVector>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QCandlestickSeries>
#include <QtCharts/QCandlestickSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include "stockdatamanager.h"

QT_BEGIN_NAMESPACE
class QObject;
class QChart;
class QLineSeries;
class QCandlestickSeries;
class QCandlestickSet;
class QValueAxis;
QT_END_NAMESPACE

QT_USE_NAMESPACE

/**
 * @brief 股票图表管理类
 * 
 * ChartManager类负责管理和显示股票数据的图表，支持分时图和K线图两种显示模式。
 * 该类管理图表的创建、更新、数据处理以及视图切换等功能。
 */
class ChartManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 图表显示类型枚举
     */
    enum ChartType {
        TimeSeries,  ///< 分时图模式
        Candlestick  ///< K线图模式
    };

    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit ChartManager(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ChartManager();

    /**
     * @brief 获取图表对象
     * @return 返回QChart指针
     */
    QChart* chart() const { return m_chart; }

    /**
     * @brief 切换图表显示类型
     * @param type 目标图表类型
     */
    void switchChartType(ChartType type);

    /**
     * @brief 清除所有图表数据
     */
    void clearData();

    /**
     * @brief 更新实时数据
     * @param data 实时股票数据
     */
    void updateRealtimeData(const StockDataManager::StockData& data);

    /**
     * @brief 更新历史数据
     * @param data 历史股票数据
     */
    void updateHistoricalData(const StockDataManager::HistoricalData& data);

private:
    /**
     * @brief 初始化图表
     * 
     * 创建并配置图表的基本设置，包括坐标轴、标题和默认显示模式
     */
    void initChart();

    /**
     * @brief 更新分时图
     * 
     * 根据当前数据更新分时图的显示
     */
    void updateTimeSeriesChart();

    /**
     * @brief 更新K线图
     * 
     * 根据当前数据更新K线图的显示，处理可视范围内的数据点
     */
    void updateCandlestickChart();

    /**
     * @brief 更新坐标轴范围
     * 
     * 根据当前数据计算并设置坐标轴的显示范围
     */
    void updateAxisRange();

    QChart* m_chart;                    ///< 图表对象指针
    QLineSeries* m_timeSeries;          ///< 分时图数据序列
    QCandlestickSeries* m_candlestickSeries;  ///< K线图数据序列
    QDateTimeAxis* m_axisX;            ///< X轴对象（时间轴）
    QValueAxis* m_axisY;                ///< Y轴对象
    ChartType m_currentType;            ///< 当前图表类型

    QVector<double> m_prices;           ///< 价格数据数组
    QVector<qint64> m_times;           ///< 时间点数组（存储时间戳）
    QVector<QCandlestickSet*> m_candlestickData;  ///< K线图数据集合
    int m_sampleCount;                  ///< 当前样本数量

    static const int MAX_VISIBLE_POINTS = 20;  ///< 最大可视数据点数
    static const int MAX_SAMPLES = 50;         ///< 最大样本数量
};

#endif // CHARTMANAGER_H
