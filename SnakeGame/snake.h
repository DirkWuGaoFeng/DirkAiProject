#ifndef SNAKE_H
#define SNAKE_H

#include <QWidget>
#include <QKeyEvent>
#include <QTimer>
#include <QPoint>
#include <QVector>
#include <QPainter>
#include <QRandomGenerator>

class Snake : public QWidget
{
    Q_OBJECT

public:
    explicit Snake(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void moveSnake();

private:
    static const int DOT_SIZE = 20;      // 蛇身每一节的大小
    static const int BOARD_WIDTH = 30;    // 游戏区域宽度（格子数）
    static const int BOARD_HEIGHT = 20;   // 游戏区域高度（格子数）
    static const int GAME_SPEED = 100;    // 游戏速度（毫秒）

    QTimer *timer;                        // 游戏定时器
    QVector<QPoint> snake;                // 蛇身坐标
    QPoint food;                          // 食物坐标
    int direction;                        // 移动方向
    bool gameOver;                        // 游戏结束标志
    int score;                            // 得分

    void initGame();                      // 初始化游戏
    void locateFood();                    // 生成食物
    void checkCollision();                // 检查碰撞
};

#endif // SNAKE_H