from typing import List, Dict, Optional
import pandas as pd
import numpy as np
from datetime import datetime
from stock_data_manager import StockDataManager

class TradingStrategy:
    def __init__(self, stock_data_manager: StockDataManager):
        self.data_manager = stock_data_manager
        self.position = 0  # 当前持仓量
        self.cash = 100000  # 初始资金
        self.trades = []  # 交易记录

    def ma_cross_strategy(self, short_period: int = 5, long_period: int = 20) -> pd.DataFrame:
        """移动平均线交叉策略
        
        Args:
            short_period: 短期移动平均线周期
            long_period: 长期移动平均线周期
        
        Returns:
            包含交易信号的DataFrame
        """
        # 获取移动平均线数据
        ma_data = self.data_manager.calculate_ma([short_period, long_period])
        
        # 计算金叉死叉信号
        ma_data['signal'] = 0
        ma_data.loc[ma_data[f'MA{short_period}'] > ma_data[f'MA{long_period}'], 'signal'] = 1
        ma_data['position'] = ma_data['signal'].diff()
        
        return ma_data

    def macd_strategy(self) -> pd.DataFrame:
        """MACD策略
        
        Returns:
            包含交易信号的DataFrame
        """
        # 获取MACD数据
        macd_data = self.data_manager.calculate_macd()
        
        # 计算交易信号
        macd_data['signal'] = 0
        # 金叉：DIF上穿DEA
        macd_data.loc[(macd_data['DIF'] > macd_data['DEA']) & 
                     (macd_data['DIF'].shift(1) <= macd_data['DEA'].shift(1)), 'signal'] = 1
        # 死叉：DIF下穿DEA
        macd_data.loc[(macd_data['DIF'] < macd_data['DEA']) & 
                     (macd_data['DIF'].shift(1) >= macd_data['DEA'].shift(1)), 'signal'] = -1
        
        return macd_data

    def rsi_strategy(self, period: int = 14, overbought: float = 70, oversold: float = 30) -> pd.DataFrame:
        """RSI策略
        
        Args:
            period: RSI计算周期
            overbought: 超买线
            oversold: 超卖线
        
        Returns:
            包含交易信号的DataFrame
        """
        # 获取RSI数据
        rsi_data = self.data_manager.calculate_rsi(period)
        
        # 计算交易信号
        rsi_data['signal'] = 0
        # 超卖买入
        rsi_data.loc[rsi_data[f'RSI{period}'] < oversold, 'signal'] = 1
        # 超买卖出
        rsi_data.loc[rsi_data[f'RSI{period}'] > overbought, 'signal'] = -1
        
        return rsi_data

    def backtest(self, strategy_data: pd.DataFrame, initial_cash: float = 100000.0) -> Dict:
        """回测策略
        
        Args:
            strategy_data: 包含交易信号的DataFrame
            initial_cash: 初始资金
        
        Returns:
            回测结果统计
        """
        cash = initial_cash
        position = 0
        trades = []
        portfolio_value = []

        for date, row in strategy_data.iterrows():
            if 'position' in row:
                signal = row['position']
            else:
                signal = row['signal']

            if signal == 1:  # 买入信号
                if cash > 0:
                    shares = int(cash / row['close'])  # 全仓买入
                    cost = shares * row['close']
                    cash -= cost
                    position += shares
                    trades.append({
                        'date': date,
                        'type': 'buy',
                        'price': row['close'],
                        'shares': shares,
                        'value': cost
                    })
            elif signal == -1:  # 卖出信号
                if position > 0:
                    value = position * row['close']
                    cash += value
                    trades.append({
                        'date': date,
                        'type': 'sell',
                        'price': row['close'],
                        'shares': position,
                        'value': value
                    })
                    position = 0

            # 记录每日组合价值
            portfolio_value.append({
                'date': date,
                'value': cash + position * row['close']
            })

        # 计算回测统计数据
        portfolio_df = pd.DataFrame(portfolio_value)
        portfolio_df.set_index('date', inplace=True)
        returns = portfolio_df['value'].pct_change()

        stats = {
            'initial_capital': initial_cash,
            'final_capital': portfolio_df['value'].iloc[-1],
            'total_return': (portfolio_df['value'].iloc[-1] - initial_cash) / initial_cash * 100,
            'annual_return': returns.mean() * 252 * 100,
            'volatility': returns.std() * np.sqrt(252) * 100,
            'sharpe_ratio': (returns.mean() * 252) / (returns.std() * np.sqrt(252)) if returns.std() != 0 else 0,
            'max_drawdown': (portfolio_df['value'] / portfolio_df['value'].cummax() - 1).min() * 100,
            'trade_count': len(trades)
        }

        return {
            'stats': stats,
            'trades': trades,
            'portfolio_value': portfolio_df.to_dict()['value']
        }