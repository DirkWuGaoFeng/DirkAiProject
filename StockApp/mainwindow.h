#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QChartView>
#include <QComboBox>
#include <QListWidget>
#include <QTableWidget>
#include <QPropertyAnimation>
#include "stockdatamanager.h"
#include "chartmanager.h"
#include "stockcodemap.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief 主窗口类
 * 
 * MainWindow类是应用程序的主窗口，负责用户界面的显示和交互。
 * 该类管理股票数据的显示、图表的展示以及用户输入的处理。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

private slots:
    /**
     * @brief 实时数据按钮点击槽函数
     * 
     * 处理用户点击实时数据按钮的事件，获取输入的股票代码并请求实时数据
     */
    void onSearchButtonClicked();

    /**
     * @brief 历史数据按钮点击槽函数
     * 
     * 处理用户点击历史数据按钮的事件，获取时间范围并请求历史数据
     */
    void onHistoryButtonClicked();

    /**
     * @brief 图表类型改变槽函数
     * @param index 选择的图表类型索引
     * 
     * 处理用户切换图表类型的事件
     */
    void onChartTypeChanged(int index);

    /**
     * @brief 股票数据接收槽函数
     * 
     * 处理接收到新的股票数据时的更新操作
     */
    void onStockDataReceived();

    /**
     * @brief 历史数据接收槽函数
     * 
     * 处理接收到历史数据时的更新操作
     */
    void onHistoricalDataReceived();

    /**
     * @brief 错误处理槽函数
     * @param error 错误信息
     * 
     * 处理数据请求或处理过程中的错误
     */
    void onErrorOccurred(const QString& error);

private:
    /**
     * @brief 事件过滤器
     * @param obj 事件对象
     * @param event 事件
     * @return 如果事件被处理返回true，否则返回false
     * 
     * 处理特定的用户界面事件
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * @brief 设置用户界面
     * 
     * 初始化和配置用户界面组件
     */
    void setupUI();

    /**
     * @brief 更新股票信息
     * @param data 股票数据
     * 
     * 更新界面上显示的股票信息
     */
    void updateStockInfo(const StockDataManager::StockData& data);

    Ui::MainWindow *ui;                  ///< UI对象指针
    QLineEdit *stockCodeEdit;            ///< 股票代码输入框
    QPushButton *searchButton;           ///< 实时数据按钮
    QPushButton *historyButton;          ///< 历史数据按钮
    QDateTimeEdit *startDateEdit;        ///< 开始时间选择框
    QDateTimeEdit *endDateEdit;          ///< 结束时间选择框
    QLabel *stockNameLabel;              ///< 股票名称标签
    QLabel *currentPriceLabel;           ///< 当前价格标签
    QLabel *totalSharesLabel;            ///< 总股本标签
    QLabel *marketValueLabel;            ///< 总市值标签
    QLabel *turnoverRateLabel;           ///< 换手率标签
    QLabel *circulatingSharesLabel;      ///< 流通股标签
    QLabel *circulatingValueLabel;       ///< 流通值标签
    QLabel *peRatioLabel;                ///< 市盈率标签
    QLabel *pbRatioLabel;                ///< 市净率标签
    QTableWidget *sellOrderTable;        ///< 卖盘数据表格
    QTableWidget *buyOrderTable;         ///< 买盘数据表格
    QChartView *chartView;               ///< 图表视图
    QComboBox *chartTypeCombo;           ///< 图表类型选择框
    QListWidget *suggestionList;         ///< 股票代码建议列表

    StockDataManager *stockManager;       ///< 股票数据管理器
    ChartManager *chartManager;           ///< 图表管理器
    StockCodeMap *stockCodeMap;           ///< 股票代码映射管理器
};

#endif // MAINWINDOW_H
