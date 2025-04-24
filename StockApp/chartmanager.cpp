#include "chartmanager.h"
#include <QDateTime>
#include <algorithm>

const int ChartManager::MAX_VISIBLE_POINTS;
const int ChartManager::MAX_SAMPLES;

/**
 * @brief 图表管理器构造函数
 * @param parent 父对象指针
 * 
 * 初始化图表管理器，创建图表对象和数据序列：
 * - 创建主图表对象
 * - 创建分时图数据序列
 * - 创建K线图数据序列
 * - 初始化采样计数和当前图表类型
 */
ChartManager::ChartManager(QObject *parent)
    : QObject(parent)
    , m_chart(new QChart())
    , m_timeSeries(new QLineSeries())
    , m_candlestickSeries(new QCandlestickSeries())
    , m_sampleCount(0)
    , m_currentType(TimeSeries)
{
    initChart();
}

ChartManager::~ChartManager()
{
    clearData();
    delete m_chart;
}

/**
 * @brief 初始化图表
 * 
 * 配置图表的基本属性和组件：
 * - 添加分时图和K线图数据序列
 * - 设置图表标题
 * - 配置X轴和Y轴
 * - 设置K线图颜色样式
 * - 设置默认显示类型为分时图
 */
void ChartManager::initChart()
{
    // 初始化图表
    m_chart->addSeries(m_timeSeries);
    m_chart->addSeries(m_candlestickSeries);
    m_chart->setTitle("股票价格走势");

    // 初始化X轴
    m_axisX = new QDateTimeAxis();
    m_axisX->setTitleText("时间");
    m_axisX->setFormat("yyyy-MM-dd hh:mm");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_timeSeries->attachAxis(m_axisX);
    m_candlestickSeries->attachAxis(m_axisX);

    // 初始化Y轴
    m_axisY = new QValueAxis();
    m_axisY->setTitleText("价格");
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_timeSeries->attachAxis(m_axisY);
    m_candlestickSeries->attachAxis(m_axisY);

    // 设置K线图颜色
    m_candlestickSeries->setIncreasingColor(QColor(Qt::red));
    m_candlestickSeries->setDecreasingColor(QColor(Qt::green));

    // 默认显示分时图
    m_timeSeries->setVisible(true);
    m_candlestickSeries->setVisible(false);
}

/**
 * @brief 切换图表类型
 * @param type 目标图表类型（TimeSeries：分时图，Candlestick：K线图）
 * 
 * 在分时图和K线图之间切换：
 * - 检查是否需要切换
 * - 更新图表显示状态
 * - 根据类型更新相应的图表数据
 */
void ChartManager::switchChartType(ChartType type)
{
    if (m_currentType == type) return;

    m_currentType = type;
    m_timeSeries->setVisible(type == TimeSeries);
    m_candlestickSeries->setVisible(type == Candlestick);

    if (type == TimeSeries) {
        updateTimeSeriesChart();
    } else {
        updateCandlestickChart();
    }
}

/**
 * @brief 清除图表数据
 * 
 * 清除所有图表相关的数据：
 * - 清除价格和时间数据
 * - 重置采样计数
 * - 清除分时图数据
 * - 清除K线图数据
 */
void ChartManager::clearData()
{
    m_prices.clear();
    m_times.clear();
    m_sampleCount = 0;
    m_timeSeries->clear();

    m_candlestickData.clear();
    m_candlestickSeries->clear();
}

/**
 * @brief 更新实时数据
 * @param data 实时股票数据
 * 
 * 处理新接收到的实时数据：
 * - 管理数据点数量，超出最大限制时移除最早的数据
 * - 添加新的数据点
 * - 根据当前图表类型更新显示
 * - 更新坐标轴范围
 */
void ChartManager::updateRealtimeData(const StockDataManager::StockData& data)
{
    if (m_sampleCount >= MAX_SAMPLES) {
        m_prices.removeFirst();
        m_times.removeFirst();
        if (m_currentType == TimeSeries) {
            m_timeSeries->clear();
            for (int i = 0; i < m_prices.size(); ++i) {
                m_timeSeries->append(m_times[i], m_prices[i]);
            }
        }
    } else {
        m_sampleCount++;
    }

    m_prices.append(data.currentPrice);
    m_times.append(data.timestamp.toMSecsSinceEpoch());

    if (m_currentType == TimeSeries) {
        m_timeSeries->append(data.timestamp.toMSecsSinceEpoch(), m_prices.last());
        updateAxisRange();
        m_chart->update();
    } else {
        QCandlestickSet *setData = new QCandlestickSet(data.openPrice, data.highPrice,
                                                   data.lowPrice, data.closePrice,
                                                   m_sampleCount);
        m_candlestickData.append(setData);

        QCandlestickSet *setSerie = new QCandlestickSet(data.openPrice, data.highPrice,
                                                   data.lowPrice, data.closePrice,
                                                   data.timestamp.toMSecsSinceEpoch());
        m_candlestickSeries->append(setSerie);
        updateCandlestickChart();
        m_chart->update();
    }
}

/**
 * @brief 更新历史数据
 * @param data 历史股票数据
 * 
 * 处理历史数据：
 * - 清除现有数据
 * - 添加所有历史数据点
 * - 更新分时图和K线图数据
 * - 根据当前图表类型更新显示
 */
void ChartManager::updateHistoricalData(const StockDataManager::HistoricalData& data)
{
    clearData();

    for (int i = 0; i < data.timestamps.size(); ++i) {
        QCandlestickSet *setData = new QCandlestickSet(data.openPrices[i],
                                                  data.highPrices[i],
                                                  data.lowPrices[i],
                                                  data.closePrices[i],
                                                  i);
        m_candlestickData.append(setData);
        QCandlestickSet *setSerie = new QCandlestickSet(data.openPrices[i],
                                                   data.highPrices[i],
                                                   data.lowPrices[i],
                                                   data.closePrices[i],
                                                   data.timestamps[i].toMSecsSinceEpoch());
        m_candlestickSeries->append(setSerie);

        m_prices.append(data.closePrices[i]);
        m_times.append(data.timestamps[i].toMSecsSinceEpoch());
        m_sampleCount++;
    }

    if (m_currentType == TimeSeries) {
        updateTimeSeriesChart();
    } else {
        updateCandlestickChart();
    }
}

/**
 * @brief 更新分时图
 * 
 * 更新分时图的显示：
 * - 清除现有数据点
 * - 重新添加所有数据点
 * - 更新坐标轴范围
 * - 刷新图表显示
 */
void ChartManager::updateTimeSeriesChart()
{
    m_timeSeries->clear();
    for (int i = 0; i < m_prices.size(); ++i) {
        m_timeSeries->append(m_times[i], m_prices[i]);
    }
    updateAxisRange();
    m_chart->update();
}

/**
 * @brief 更新K线图
 * 
 * 更新K线图的显示：
 * - 检查数据是否为空
 * - 计算可见数据点范围
 * - 更新X轴和Y轴范围
 * - 调整显示比例和刻度
 */
void ChartManager::updateCandlestickChart()
{
    if (m_candlestickData.isEmpty()) return;

    int dataSize = m_candlestickData.size();
    int visiblePoints = std::min(MAX_VISIBLE_POINTS, dataSize);
    int startPoint = std::max(0, dataSize - visiblePoints);

    // 更新X轴范围
    QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(m_candlestickSeries->sets().at(startPoint)->timestamp());
    QDateTime lastTime = QDateTime::fromMSecsSinceEpoch(m_candlestickSeries->sets().last()->timestamp());
    m_axisX->setRange(firstTime, lastTime);
    m_axisX->setTickCount(std::min(10, visiblePoints));

    // 更新Y轴范围
    QVector<double> visibleHighPrices;
    QVector<double> visibleLowPrices;
    for (int i = startPoint; i < dataSize; ++i) {
        QCandlestickSet* set = m_candlestickData[i];
        visibleHighPrices.append(set->high());
        visibleLowPrices.append(set->low());
    }

    double minPrice = *std::min_element(visibleLowPrices.begin(), visibleLowPrices.end());
    double maxPrice = *std::max_element(visibleHighPrices.begin(), visibleHighPrices.end());
    double margin = (maxPrice - minPrice) * 0.1;
    m_axisY->setRange(minPrice - margin, maxPrice + margin);
}

/**
 * @brief 更新坐标轴范围
 * 
 * 根据当前数据更新坐标轴的显示范围：
 * - 更新X轴范围
 * - 计算并设置Y轴范围
 * - 添加边距确保数据点显示完整
 */
void ChartManager::updateAxisRange()
{
    if (m_prices.isEmpty()) return;

    // 更新X轴范围
    QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(m_times.first());
    QDateTime lastTime = QDateTime::fromMSecsSinceEpoch(m_times.last());
    m_axisX->setRange(firstTime, lastTime);

    // 更新Y轴范围
    double minPrice = *std::min_element(m_prices.begin(), m_prices.end());
    double maxPrice = *std::max_element(m_prices.begin(), m_prices.end());
    double margin = (maxPrice - minPrice) * 0.1;
    m_axisY->setRange(minPrice - margin, maxPrice + margin);
}
