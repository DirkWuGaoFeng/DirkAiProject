import akshare as ak
import pymysql

# 从 akshare 获取 A 股所有股票代码
def fetch_a_stock_codes():
    stock_list = ak.stock_info_a_code_name()
    return stock_list

#将akshare获取的股票代码转换为 sz000001 这种格式
def convert_stock_code(code):
    if code.startswith('6'):
        converted_code = 'sh' + code
    elif code.startswith('0') or code.startswith('3'):
        converted_code = 'sz' + code
    else:
        converted_code = 'bj' + code
    return converted_code

#将股票代码和名称保存到数据库 stock_code 表中
def save_stock_codes_to_database(stock_list):
    # 连接数据库
    conn = pymysql.connect(host='localhost', user='root', password='123456', db='mysql', charset='utf8')
    cursor = conn.cursor()

    # 创建 stock_code 表
    cursor.execute("CREATE TABLE IF NOT EXISTS stock_code (code VARCHAR(255), name VARCHAR(255))")
    # 清空 stock_code 表
    cursor.execute("TRUNCATE TABLE stock_code")

    # 插入股票代码和名称            
    for _, row in stock_list.iterrows():
        sql = "INSERT INTO stock_code (code, name) VALUES (%s, %s)"
        code = convert_stock_code(row['code'])
        cursor.execute(sql, (code, row['name']))

    # 提交事务
    conn.commit()

    # 关闭数据库连接
    cursor.close()
    conn.close()

if __name__ == "__main__":
    stock_list = fetch_a_stock_codes()
    #print(stock_list)
    save_stock_codes_to_database(stock_list)
    