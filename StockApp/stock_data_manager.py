import requests
import pandas as pd
import numpy as np
from datetime import datetime
from typing import Dict, List, Optional

class StockDataManager:
    def __init__(self):
        self.current_stock_code = ""
        self.historical_data = pd.DataFrame()
        self.latest_data = {}

    def request_stock_data(self, stock_code: str) -> None:
        """请求股票数据，包括历史K线和实时数据"""
        # 验证股票代码格式
        if not (stock_code.startswith("sh") or stock_code.startswith("sz")):
            raise ValueError("股票代码格式错误：必须以'sh'或'sz'开头")
        
        code = stock_code[2:]
        if len(code) != 6 or not code.isdigit():
            raise ValueError("股票代码格式错误：必须为6位数字")

        self.current_stock_code = stock_code
        self._get_historical_data()
        self._get_realtime_data()

    def _get_historical_data(self) -> None:
        """获取历史K线数据"""
        url = f"http://web.ifzq.gtimg.cn/appstock/app/fqkline/get?param={self.current_stock_code},day,,,100,qfq"
        headers = {"User-Agent": "Mozilla/5.0"}
        
        try:
            response = requests.get(url, headers=headers)
            response.raise_for_status()
            data = response.json()
            
            if "data" not in data:
                raise ValueError("JSON数据缺少data字段")
                
            stock_data = data["data"][self.current_stock_code]
            if "qfqday" not in stock_data:
                raise ValueError("JSON数据缺少qfqday字段")
                
            daily_data = stock_data["qfqday"]
            if not daily_data:
                raise ValueError("历史数据为空")

            # 转换为DataFrame格式
            df_data = {
                "date": [],
                "open": [],
                "close": [],
                "high": [],
                "low": []
            }

            for item in daily_data:
                df_data["date"].append(pd.to_datetime(item[0]))
                df_data["open"].append(float(item[1]))
                df_data["close"].append(float(item[2]))
                df_data["high"].append(float(item[3]))
                df_data["low"].append(float(item[4]))

            self.historical_data = pd.DataFrame(df_data)
            self.historical_data.set_index("date", inplace=True)

        except requests.RequestException as e:
            raise ConnectionError(f"获取历史数据失败: {str(e)}")
        except (ValueError, KeyError) as e:
            raise ValueError(f"解析历史数据失败: {str(e)}")

    def _get_realtime_data(self) -> None:
        """获取实时数据"""
        url = f"http://hq.sinajs.cn/list={self.current_stock_code}"
        headers = {
            "User-Agent": "Mozilla/5.0",
            "Referer": "http://finance.sina.com.cn/"
        }

        try:
            response = requests.get(url, headers=headers)
            response.raise_for_status()
            text = response.text

            # 解析数据
            data_str = text.split('"')[1]
            if not data_str:
                raise ValueError("获取实时数据失败")

            fields = data_str.split(',')
            if len(fields) < 32:
                raise ValueError("实时数据格式错误")

            self.latest_data = {
                "name": fields[0],
                "current_price": float(fields[3]),
                "open_price": float(fields[1]),
                "high_price": float(fields[4]),
                "low_price": float(fields[5]),
                "close_price": float(fields[2]),  # 昨收价
                "timestamp": datetime.strptime(f"{fields[30]} {fields[31]}", "%Y-%m-%d %H:%M:%S")
            }

        except requests.RequestException as e:
            raise ConnectionError(f"获取实时数据失败: {str(e)}")
        except (ValueError, IndexError) as e:
            raise ValueError(f"解析实时数据失败: {str(e)}")

    def calculate_ma(self, periods: List[int]) -> pd.DataFrame:
        """计算移动平均线
        
        Args:
            periods: 需要计算的周期列表，如[5, 10, 20]
        
        Returns:
            包含移动平均线的DataFrame
        """
        result = self.historical_data.copy()
        for period in periods:
            result[f'MA{period}'] = self.historical_data['close'].rolling(window=period).mean()
        return result

    def calculate_macd(self) -> pd.DataFrame:
        """计算MACD指标
        
        Returns:
            包含MACD指标的DataFrame
        """
        result = self.historical_data.copy()
        # 计算快速和慢速EMA
        result['EMA12'] = self.historical_data['close'].ewm(span=12, adjust=False).mean()
        result['EMA26'] = self.historical_data['close'].ewm(span=26, adjust=False).mean()
        # 计算DIF
        result['DIF'] = result['EMA12'] - result['EMA26']
        # 计算DEA
        result['DEA'] = result['DIF'].ewm(span=9, adjust=False).mean()
        # 计算MACD
        result['MACD'] = 2 * (result['DIF'] - result['DEA'])
        return result

    def calculate_rsi(self, period: int = 14) -> pd.DataFrame:
        """计算RSI指标
        
        Args:
            period: RSI计算周期，默认14天
        
        Returns:
            包含RSI指标的DataFrame
        """
        result = self.historical_data.copy()
        delta = result['close'].diff()
        gain = (delta.where(delta > 0, 0)).rolling(window=period).mean()
        loss = (-delta.where(delta < 0, 0)).rolling(window=period).mean()
        rs = gain / loss
        result[f'RSI{period}'] = 100 - (100 / (1 + rs))
        return result