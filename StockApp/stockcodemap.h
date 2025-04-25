#ifndef STOCKCODEMAP_H
#define STOCKCODEMAP_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSqlError>

/**
 * @brief 股票代码映射管理类
 * 
 * 该类管理股票代码和名称的映射关系，提供股票代码查询和搜索功能。
 * 支持通过股票代码或中文名称进行模糊匹配搜索。
 */
class StockCodeMap : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit StockCodeMap(QObject *parent = nullptr);

    /**
     * @brief 初始化股票代码映射表
     */
    void initialize();

    /**
     * @brief 连接到 MySQL 数据库
     */
    bool connectToMySQL();

    /**
     * @brief 获取股票数据表
     */
    void fetchStockData();

    /**
     * @brief 搜索股票
     * @param keyword 搜索关键词（代码或名称）
     * @return 匹配的股票列表（格式：代码 - 名称）
     */
    QStringList search(const QString& keyword) const;

    /**
     * @brief 获取股票名称
     * @param code 股票代码
     * @return 股票名称
     */
    QString getStockName(const QString& code) const;

    /**
     * @brief 获取股票代码
     * @param name 股票名称
     * @return 股票代码
     */
    QString getStockCode(const QString& name) const;

private:
    QMap<QString, QString> codeToName;  ///< 股票代码到名称的映射
    QMap<QString, QString> nameToCode;  ///< 股票名称到代码的映射
};

#endif // STOCKCODEMAP_H
