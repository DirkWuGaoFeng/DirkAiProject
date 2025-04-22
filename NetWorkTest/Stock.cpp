#include <iostream>
#include <boost/asio.hpp>
//#include <boost/asio/ssl.hpp>
#include <string>
#include <thread>
#include <chrono>
#include <windows.h> // 添加头文件以支持编码转换
#include "matplotlibcpp.h" // 添加matplotlib-cpp头文件
#include <vector>

namespace plt = matplotlibcpp; // 添加命名空间

std::vector<double> prices; // 添加全局变量
std::vector<int> times;     // 添加全局变量
using boost::asio::ip::tcp;
using namespace std;

// 添加GBK转UTF-8的辅助函数
string gbk_to_utf8(const string& gbk_str) {
    int len = MultiByteToWideChar(CP_ACP, 0, gbk_str.c_str(), -1, NULL, 0);
    wstring wstr(len, 0);
    MultiByteToWideChar(CP_ACP, 0, gbk_str.c_str(), -1, &wstr[0], len);

    len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    string utf8_str(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], len, NULL, NULL);

    // 去掉最后的\0
    if (!utf8_str.empty() && utf8_str.back() == '\0') utf8_str.pop_back();
    return utf8_str;
}

void getStockInfo(const string& code, int count) {
    try {
        boost::asio::io_context io_context;
        tcp::resolver resolver(io_context);
        tcp::resolver::results_type endpoints = resolver.resolve("hq.sinajs.cn", "80");

        tcp::socket socket(io_context);
        boost::asio::connect(socket, endpoints);

        string request = "GET /list=" + code + " HTTP/1.1\r\n";
        request += "Host: hq.sinajs.cn\r\n";
        request += "User-Agent: Mozilla/5.0\r\n";
        request += "Referer: http://finance.sina.com.cn/\r\n"; // 新增Referer
        request += "Connection: close\r\n\r\n";

        boost::asio::write(socket, boost::asio::buffer(request));

        boost::asio::streambuf response;
        boost::system::error_code error;
        boost::asio::read_until(socket, response, "\r\n\r\n", error);

        istream response_stream(&response);
        string header;
        while (getline(response_stream, header) && header != "\r") {}

        string line;
        while (getline(response_stream, line)) {
            string utf8_line = gbk_to_utf8(line);
            cout << "股票行情原始数据：" << utf8_line << endl;

            // 解析价格
            size_t pos1 = utf8_line.find("\"");
            size_t pos2 = utf8_line.rfind("\"");
            if (pos1 != string::npos && pos2 != string::npos && pos2 > pos1) {
                string data = utf8_line.substr(pos1 + 1, pos2 - pos1 - 1);
                vector<string> fields;
                size_t start = 0, end;
                while ((end = data.find(",", start)) != string::npos) {
                    fields.push_back(data.substr(start, end - start));
                    start = end + 1;
                }
                fields.push_back(data.substr(start));
                // fields[3]为当前价
                if (fields.size() > 3) {
                    prices.push_back(atof(fields[3].c_str()));
                    times.push_back(count);
                }
            }
        }
    } catch (std::exception& e) {
        cout << "获取行情失败: " << e.what() << endl;
    }
}

int main() {
    string code;
    cout << "请输入股票代码（如 sh600000 或 sz000001）: ";
    cin >> code;
    int count = 0;

    plt::rcparams({{"font.sans-serif", "SimHei"}, {"axes.unicode_minus", "False"}}); // 设置中文字体
    plt::ion(); // 开启交互模式
    while (true) {
        getStockInfo(code, count);

        // 绘制实时曲线
        plt::clf();
        plt::title("股票价格实时曲线");
        plt::xlabel("采样点");
        plt::ylabel("价格");
        plt::plot(times, prices, "b-o");
        plt::pause(0.01);

        this_thread::sleep_for(chrono::seconds(5));
        cout << "-----------------------------" << endl;

        if (count == 10) {
            break;
        }
        count++;
    }
    // plt::ioff(); // 删除这一行
    plt::show();
    return 0;
}