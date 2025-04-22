#include "snake.h"

Snake::Snake(QWidget *parent) : QWidget(parent)
{
    setFixedSize(BOARD_WIDTH * DOT_SIZE, BOARD_HEIGHT * DOT_SIZE);
    setWindowTitle("贪吃蛇游戏");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Snake::moveSnake);

    initGame();
}

void Snake::initGame()
{
    // 初始化蛇的位置（从中间开始）
    snake.clear();
    snake.append(QPoint(BOARD_WIDTH / 2, BOARD_HEIGHT / 2));
    snake.append(QPoint(BOARD_WIDTH / 2 - 1, BOARD_HEIGHT / 2));
    snake.append(QPoint(BOARD_WIDTH / 2 - 2, BOARD_HEIGHT / 2));

    // 初始化方向（向右）
    direction = 0;
    
    // 初始化游戏状态
    gameOver = false;
    score = 0;

    // 生成第一个食物
    locateFood();

    // 启动定时器
    timer->start(GAME_SPEED);
}

void Snake::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (gameOver) {
        // 游戏结束显示
        QString message = QString("游戏结束！得分: %1").arg(score);
        painter.setPen(Qt::red);
        painter.setFont(QFont("Arial", 15));
        painter.drawText(rect(), Qt::AlignCenter, message);
        return;
    }

    // 绘制食物
    painter.fillRect(food.x() * DOT_SIZE, food.y() * DOT_SIZE,
                     DOT_SIZE, DOT_SIZE, Qt::red);

    // 绘制蛇
    for (int i = 0; i < snake.size(); ++i) {
        if (i == 0) {
            // 蛇头
            painter.fillRect(snake[i].x() * DOT_SIZE, snake[i].y() * DOT_SIZE,
                            DOT_SIZE, DOT_SIZE, Qt::darkGreen);
        } else {
            // 蛇身
            painter.fillRect(snake[i].x() * DOT_SIZE, snake[i].y() * DOT_SIZE,
                            DOT_SIZE, DOT_SIZE, Qt::green);
        }
    }

    // 绘制得分
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 10));
    painter.drawText(5, 15, QString("得分: %1").arg(score));
}

void Snake::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        if (direction != 0) direction = 2;
        break;
    case Qt::Key_Right:
        if (direction != 2) direction = 0;
        break;
    case Qt::Key_Up:
        if (direction != 1) direction = 3;
        break;
    case Qt::Key_Down:
        if (direction != 3) direction = 1;
        break;
    case Qt::Key_Space:
        if (gameOver) initGame();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void Snake::moveSnake()
{
    if (gameOver) return;

    // 获取蛇头位置
    QPoint head = snake[0];

    // 根据方向移动蛇头
    switch (direction) {
    case 0: // 右
        head.rx()++;
        break;
    case 1: // 下
        head.ry()++;
        break;
    case 2: // 左
        head.rx()--;
        break;
    case 3: // 上
        head.ry()--;
        break;
    }

    // 检查是否吃到食物
    if (head == food) {
        snake.prepend(head);
        score += 10;
        locateFood();
    } else {
        snake.prepend(head);
        snake.removeLast();
    }

    // 检查碰撞
    checkCollision();

    // 更新界面
    update();
}

void Snake::locateFood()
{
    int x, y;
    bool validPosition;

    do {
        validPosition = true;
        x = QRandomGenerator::global()->bounded(BOARD_WIDTH);
        y = QRandomGenerator::global()->bounded(BOARD_HEIGHT);

        // 确保食物不会出现在蛇身上
        for (const QPoint &point : snake) {
            if (point == QPoint(x, y)) {
                validPosition = false;
                break;
            }
        }
    } while (!validPosition);

    food = QPoint(x, y);
}

void Snake::checkCollision()
{
    // 获取蛇头位置
    QPoint head = snake[0];

    // 检查是否撞墙
    if (head.x() < 0 || head.x() >= BOARD_WIDTH ||
        head.y() < 0 || head.y() >= BOARD_HEIGHT) {
        gameOver = true;
        timer->stop();
        return;
    }

    // 检查是否撞到自己
    for (int i = 1; i < snake.size(); ++i) {
        if (snake[i] == head) {
            gameOver = true;
            timer->stop();
            return;
        }
    }
}