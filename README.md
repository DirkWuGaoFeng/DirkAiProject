# DirkAi Project

## 项目简介
DirkAi Project 是一个多功能的C++/Python项目集合，包含了贪吃蛇游戏、股票分析工具等应用程序。本项目展示了现代C++编程实践，以及C++与Python的混合开发方案。

## 项目结构
- **SnakeGame**: 基于Qt6开发的经典贪吃蛇游戏
- **StockApp**: 股票数据分析和交易策略工具
- **NetWorkTest**: 网络测试工具

## 主要功能

### 贪吃蛇游戏 (SnakeGame)
- 使用Qt6框架开发的图形界面
- 经典的贪吃蛇游戏玩法
- 现代C++17实现

### 股票分析工具 (StockApp)
- 股票数据获取和管理
- 交易策略实现
- 图表可视化
- Python与C++混合开发

## 环境要求

### SnakeGame
- CMake 3.16+
- Qt6
- C++17兼容的编译器

### StockApp
- Python 3.11+
- C++编译器
- Qt框架

Python依赖：
```
pandas >= 2.0.0
numpy >= 1.24.0
requests >= 2.31.0
akshare >= 1.7.0
```

## 构建说明

### SnakeGame
```bash
cd SnakeGame
mkdir build && cd build
cmake ..
make
```

### StockApp
1. 安装Python依赖：
```bash
cd StockApp
pip install -r requirements.txt
```

2. 构建C++部分：
```bash
mkdir build && cd build
cmake ..
make
```

## 开发说明
- 使用CMake进行项目管理
- 遵循现代C++编程规范
- 采用Python进行数据分析和策略实现
- Qt框架用于GUI开发

## 许可证
本项目采用MIT许可证
