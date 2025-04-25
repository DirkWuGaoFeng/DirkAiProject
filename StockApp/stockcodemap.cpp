/**
 * @file stockcodemap.cpp
 * @brief 股票代码映射管理类的实现文件
 * 
 * 该文件实现了StockCodeMap类的所有功能，包括股票代码和名称的映射管理、
 * 模糊搜索匹配等功能。
 */

#include "stockcodemap.h"

StockCodeMap::StockCodeMap(QObject *parent)
    : QObject(parent)
{
    initialize();
}

void StockCodeMap::initialize()
{
    if (connectToMySQL()) {
        fetchStockData();
    }
}

// 连接到 MySQL 数据库
bool StockCodeMap::connectToMySQL() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("mysql");
    db.setUserName("root");
    db.setPassword("123456");

    if (!db.open()) {
        auto error = db.lastError();
        qDebug() << "Failed to connect to MySQL database:" << db.lastError().text();
        return false;
    }
    return true;
}

// 从数据库获取股票名称和代码
void StockCodeMap::fetchStockData() {
    QSqlQuery query;
    query.exec("SELECT code, name FROM stock_code");
    while (query.next()) {
        QString code = query.value("code").toString();
        QString name = query.value("name").toString();
        nameToCode[name] = code;
        codeToName[code] = name;
        //qDebug() << "股票代码:" << ts_code << "股票名称:" << name;
    }
}

QStringList StockCodeMap::search(const QString& keyword) const
{
    QStringList results;
    QString loweredKeyword = keyword.toLower();

    // 搜索股票代码
    for (auto it = codeToName.begin(); it != codeToName.end(); ++it) {
        if (it.key().contains(loweredKeyword) || 
            it.value().contains(keyword)) {
            results.append(QString("%1 - %2").arg(it.key()).arg(it.value()));
        }
    }

    return results;
}

QString StockCodeMap::getStockName(const QString& code) const
{
    return codeToName.value(code.toLower());
}

QString StockCodeMap::getStockCode(const QString& name) const
{
    return nameToCode.value(name);
}
