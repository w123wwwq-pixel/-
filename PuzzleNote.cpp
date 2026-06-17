// ===================== 引入头文件（相当于工具箱，每个头文件提供不同的工具） =====================
#include <iostream>      // 标准输入输出工具箱（cout/cin）
#include <vector>        // 动态数组工具箱（可以自动变长的"盒子"，用来存拼图块、图片）
#include <string>        // 字符串工具箱（处理文字）
#include <ctime>         // 时间工具箱（生成随机数、计时）
#include <graphics.h>    // EasyX图形工具箱（所有绘图、窗口功能）
#include <windows.h>     // Windows系统工具箱（弹窗、获取窗口句柄）

// 使用标准命名空间，不用每次写std::（简化代码）
using namespace std;


// ===================== 1. 拼图块实体类（纯数据类，只存单个拼图块的信息） =====================
// 作用：就像拼图的每一块，只记录它的编号、位置、是不是空白块，没有任何业务逻辑
class Piece {
private:
    int id;         // 拼图块唯一编号（0-8对应3×3，0-15对应4×4）
    int row;        // 拼图块当前在第几行
    int col;        // 拼图块当前在第几列
    bool isBlank;   // 是不是空白块（最后一块是空白的，用来移动其他块）

public:
    // 无参构造函数：创建一个默认的拼图块
    Piece() : id(0), row(0), col(0), isBlank(false) {}

    // 有参构造函数：创建拼图块时直接赋值
    Piece(int id, int r, int c, bool blank = false)
        : id(id), row(r), col(c), isBlank(blank) {
    }

    // 获取拼图块编号（getter方法，因为id是私有的，外部不能直接访问）
    int getId() const { return id; }
    // 获取拼图块当前行
    int getRow() const { return row; }
    // 获取拼图块当前列
    int getCol() const { return col; }
    // 判断是不是空白块
    bool getIsBlank() const { return isBlank; }

    // 设置拼图块所在行（setter方法）
    void setRow(int r) { row = r; }
    // 设置拼图块所在列
    void setCol(int c) { col = c; }
};


// ===================== 2. 计时器类（专门负责游戏计时，独立可复用） =====================
// 作用：记录游戏开始到现在的时间，通关后锁定时间
class GameTimer {
private:
    clock_t startTime;    // 游戏开始时的时间戳（毫秒）
    bool running;         // 计时器是不是正在运行
    int savedSeconds;     // 停止计时后，保存的最终通关时间（秒）

public:
    // 构造函数：初始化计时器状态
    GameTimer() : startTime(0), running(false), savedSeconds(0) {}

    // 开始计时：游戏开始时调用
    void start() {
        startTime = clock();       // 记录当前时间作为开始时间
        running = true;            // 标记计时器正在运行
        savedSeconds = 0;          // 重置保存的时间
    }

    // 停止计时：通关时调用，锁定最终时间
    void stop() {
        if (running) { // 只有正在运行时才停止
            // 计算从开始到现在的秒数，保存起来
            savedSeconds = (int)(clock() - startTime) / CLOCKS_PER_SEC;
            running = false; // 标记计时器停止
        }
    }

    // 重置计时器：返回主菜单时调用
    void reset() {
        startTime = 0;
        running = false;
        savedSeconds = 0;
    }

    // 获取当前已用时间（秒）
    int getSeconds() const {
        if (running) {
            // 如果正在运行，实时计算时间
            return (int)(clock() - startTime) / CLOCKS_PER_SEC;
        }
        // 如果已经停止，返回之前保存的最终时间
        return savedSeconds;
    }

    // 获取格式化的时间字符串（MM:SS格式，比如01:23）
    wstring getTimeString() const {
        int sec = getSeconds();
        int min = sec / 60; // 计算分钟
        sec %= 60;          // 计算秒（减去分钟后剩下的）
        wchar_t buf[20];    // 定义字符数组存格式化后的字符串
        // 格式化输出，不足两位补0（比如5秒变成00:05）
        swprintf_s(buf, L"%02d:%02d", min, sec);
        return wstring(buf); // 转成宽字符串返回
    }
};


// ===================== 3. 棋盘核心逻辑类（处理所有游戏规则） =====================
// 作用：管理拼图的移动、打乱、通关判断，不涉及任何绘图
class Board {
private:
    int rows;               // 棋盘总行数
    int cols;               // 棋盘总列数
    int pieceSize;          // 单个拼图块的大小（像素）
    vector<Piece> pieces;   // 存所有拼图块的"盒子"
    int blankRow;           // 空白块当前所在行
    int blankCol;           // 空白块当前所在列

    // 私有方法：交换两个拼图块的位置（核心移动逻辑）
    void swapPieces(int r1, int c1, int r2, int c2) {
        // 计算两个块在vector数组中的索引（行×列数 + 列）
        int idx1 = r1 * cols + c1;
        int idx2 = r2 * cols + c2;
        // 交换数组中两个拼图块的位置
        swap(pieces[idx1], pieces[idx2]);
        // 更新两个块的坐标
        pieces[idx1].setRow(r1);
        pieces[idx1].setCol(c1);
        pieces[idx2].setRow(r2);
        pieces[idx2].setCol(c2);
        // 空白块移动到了(r1,c1)的位置，更新空白块坐标
        blankRow = r1;
        blankCol = c1;
    }

public:
    // 构造函数：创建指定大小的棋盘
    Board(int r, int c, int size) : rows(r), cols(c), pieceSize(size) {
        init(); // 初始化棋盘
    }

    // 初始化棋盘：生成有序的拼图块（12345678空）
    void init() {
        pieces.clear(); // 先清空之前的拼图块
        // 遍历所有行和列
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                int id = i * cols + j; // 计算块的编号
                // 最后一个块是空白块
                bool blank = (i == rows - 1 && j == cols - 1);
                // 创建拼图块并加入vector盒子
                pieces.emplace_back(id, i, j, blank);
            }
        }
        // 初始化空白块在右下角
        blankRow = rows - 1;
        blankCol = cols - 1;
    }

    // 打乱拼图：通过随机移动空白块实现，天然保证拼图可解
    void shuffle() {
        srand((unsigned)time(NULL)); // 设置随机数种子为当前时间，保证每次打乱都不一样
        // 随机移动2000次，足够打乱
        for (int i = 0; i < 2000; i++) {
            int r = rand() % rows; // 随机生成一个行
            int c = rand() % cols; // 随机生成一个列
            clickPiece(r, c);      // 尝试点击这个块，能移动就移动
        }
    }

    // 点击拼图块，尝试移动（返回true表示移动成功）
    bool clickPiece(int r, int c) {
        // 检查行和列是不是在合法范围内（不能点到棋盘外面）
        if (r < 0 || r >= rows || c < 0 || c >= cols) return false;
        // 判断点击的块是不是和空白块相邻（上下左右）
        bool adjacent = (abs(r - blankRow) == 1 && c == blankCol) || // 上下相邻
            (abs(c - blankCol) == 1 && r == blankRow);              // 左右相邻
        // 如果相邻，交换两个块的位置
        if (adjacent) {
            swapPieces(r, c, blankRow, blankCol);
            return true;
        }
        // 不相邻，无法移动
        return false;
    }

    // 判断是否通关：所有拼图块都在正确的位置
    bool isComplete() const {
        // 遍历所有拼图块
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++) {
                int idx = i * cols + j; // 计算当前位置应该有的块编号
                if (pieces[idx].getIsBlank()) continue; // 跳过空白块
                // 如果块的实际编号和应该有的编号不一样，说明没通关
                if (pieces[idx].getId() != idx) return false;
            }
        // 所有块都在正确位置，通关
        return true;
    }

    // 获取指定位置的拼图块
    const Piece& getPiece(int r, int c) const { return pieces[r * cols + c]; }
    // 获取棋盘行数
    int getRows() const { return rows; }
    // 获取棋盘列数
    int getCols() const { return cols; }
    // 获取单个拼图块大小
    int getPieceSize() const { return pieceSize; }
};


// ===================== 4. 图像管理类（✅ 你负责的模块） =====================
// 作用：所有图片相关的功能：加载、切割、绘制、默认块生成
class ImageManager {
private:
    IMAGE originalImg;          // 存完整的原始图片（你选的猫的照片）
    vector<IMAGE> pieceImgs;    // 存切割后的所有拼图块图片的盒子
    int rows;                   // 拼图行数
    int cols;                   // 拼图列数
    int pieceSize;              // 单个拼图块大小

    // 私有方法：生成默认的彩色数字拼图块（图片加载失败时调用）
    void generateDefault() {
        pieceImgs.clear(); // 清空之前的拼图块图片
        // 准备9种不同的颜色，循环使用
        COLORREF colors[] = {
            RGB(255, 180, 180), RGB(180, 255, 180), RGB(180, 180, 255),
            RGB(255, 255, 180), RGB(255, 180, 255), RGB(180, 255, 255),
            RGB(255, 220, 180), RGB(220, 180, 255), RGB(180, 220, 255)
        };

        // 循环生成所有拼图块
        for (int i = 0; i < rows * cols; i++) {
            // 1. 拿一张空白的纸，大小是pieceSize×pieceSize
            IMAGE block(pieceSize, pieceSize);
            // 2. 告诉EasyX：接下来我要在这张空白纸上画画，不是在屏幕上画
            SetWorkingImage(&block);

            // 3. 给这张纸涂背景色（i%9循环使用9种颜色）
            setfillcolor(colors[i % 9]);
            solidrectangle(0, 0, pieceSize, pieceSize); // 画一个填满颜色的正方形
            // 4. 给正方形画黑色边框
            setlinecolor(RGB(60, 60, 60));
            rectangle(0, 0, pieceSize - 1, pieceSize - 1);

            // 5. 在正方形中间写数字（i+1，从1开始显示）
            wchar_t num[10];
            swprintf_s(num, L"%d", i + 1); // 把数字转成字符串
            settextstyle(pieceSize / 2, 0, L"微软雅黑"); // 设置字体大小
            settextcolor(BLACK); // 文字颜色黑色
            setbkmode(TRANSPARENT); // 文字背景透明，不然会盖住底色
            // 计算文字居中的坐标：(正方形宽度-文字宽度)/2
            int tx = (pieceSize - textwidth(num)) / 2;
            int ty = (pieceSize - textheight(num)) / 2;
            outtextxy(tx, ty, num); // 在(tx,ty)位置写数字

            // 6. 告诉EasyX：画完了，接下来还是在屏幕上画画
            SetWorkingImage(NULL);
            // 7. 把画好的这张数字块，放进pieceImgs盒子里
            pieceImgs.push_back(block);
        }
    }

public:
    // 构造函数：初始化图像管理器
    ImageManager() : rows(0), cols(0), pieceSize(0) {}

    // 加载图片并切割成拼图块
    void loadImage(wstring path, int r, int c, int size) {
        rows = r; cols = c; pieceSize = size;
        pieceImgs.clear();

        // 计算整张拼图的总大小（3×3就是360×360）
        int totalW = cols * pieceSize;
        int totalH = rows * pieceSize;

        // 1. 加载你选的图片，自动缩放到360×360大小
        int ret = loadimage(&originalImg, path.c_str(), totalW, totalH, true);
        // ✅ 兜底逻辑：如果图片加载失败（文件不存在、格式错），就生成数字块
        if (ret != 0) {
            generateDefault();
            return;
        }

        // 2. 开始切割图片
        SetWorkingImage(&originalImg); // 告诉EasyX：接下来在这张原图上操作
        // 遍历所有行和列
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // 拿一张空白纸
                IMAGE block(pieceSize, pieceSize);
                // 从原图的(j*pieceSize, i*pieceSize)位置，抠一块pieceSize大小的图
                getimage(&block, j * pieceSize, i * pieceSize, pieceSize, pieceSize);

                // 给抠出来的小图加边框
                SetWorkingImage(&block);
                setlinecolor(RGB(80, 80, 80));
                rectangle(0, 0, pieceSize - 1, pieceSize - 1);
                SetWorkingImage(&originalImg); // 切回原图继续抠下一块

                // 放进盒子里
                pieceImgs.push_back(block);
            }
        }
        SetWorkingImage(NULL); // 切回屏幕
    }

    // 把第id号拼图块，贴到屏幕的(x,y)位置
    void drawPiece(int id, int x, int y) const {
        if (id >= 0 && id < (int)pieceImgs.size()) // 检查id是否合法
            putimage(x, y, &pieceImgs[id]); // EasyX的贴图片函数
    }

    // 把整张原图，贴到屏幕的(x,y)位置，大小是w×h（原图预览功能）
    void drawOriginal(int x, int y, int w, int h) const {
        putimage(x, y, w, h, &originalImg, 0, 0);
    }
};


// ===================== 5. UI管理类（✅ 你负责的模块） =====================
// 作用：所有界面绘制和按钮点击检测，不包含任何游戏逻辑
class UIManager {
private:
    int winW, winH;               // 整个窗口的宽和高（500×520）
    int boardX, boardY;           // 拼图棋盘的左上角坐标（70,80）

    // 主菜单按钮坐标和尺寸
    int btnStartX, btnStartY, btnStartW, btnStartH; // 开始游戏按钮
    int btnDiff1X, btnDiff1Y, btnDiffW, btnDiffH;   // 简单难度按钮
    int btnDiff2X, btnDiff2Y;                       // 中等难度按钮
    int btnDiff3X, btnDiff3Y;                       // 困难难度按钮

    // 游戏内按钮坐标和尺寸
    int btnPreviewX, btnPreviewY, btnPreviewW, btnPreviewH; // 原图预览按钮
    int btnBackX, btnBackY, btnBackW, btnBackH;             // 返回菜单按钮

public:
    // 构造函数：提前算好所有按钮的位置，保证界面居中好看
    UIManager(int w, int h) : winW(w), winH(h) {
        boardX = 70;  // 棋盘左上角离窗口左边70像素
        boardY = 80;  // 棋盘左上角离窗口上边80像素

        // 开始游戏按钮：宽200高50，水平居中
        btnStartW = 200; btnStartH = 50;
        btnStartX = (winW - btnStartW) / 2; // 居中公式：(窗口宽度-按钮宽度)/2
        btnStartY = 180; // 离窗口上边180像素

        // 三个难度按钮：每个宽100高40，均匀分布
        btnDiffW = 100; btnDiffH = 40;
        btnDiff1X = winW / 2 - 160; // 第一个在中间偏左160
        btnDiff2X = winW / 2 - 50;  // 第二个在中间偏左50
        btnDiff3X = winW / 2 + 60;  // 第三个在中间偏右60
        btnDiff1Y = btnDiff2Y = btnDiff3Y = 280; // 三个按钮在同一行

        // 游戏内按钮
        btnPreviewW = 120; btnPreviewH = 35;
        btnBackW = 120; btnBackH = 35;
        btnPreviewX = boardX; // 预览按钮在棋盘左下角
        btnBackX = boardX + 360 - btnBackW; // 返回按钮在棋盘右下角
        btnPreviewY = btnBackY = boardY + 360 + 20; // 两个按钮在同一行
    }

    // 绘制背景：擦黑板，用浅灰色填满整个窗口
    void drawBackground() const {
        setbkcolor(RGB(245, 245, 245)); // 设置背景色为浅灰色
        cleardevice(); // 清屏
    }

    // 绘制主菜单界面
    void drawMenu(int selectDiff) const {
        // 1. 画顶部大标题 "拼图小游戏"
        settextstyle(48, 0, L"微软雅黑"); // 48号字，微软雅黑
        settextcolor(RGB(50, 50, 80)); // 深灰色
        setbkmode(TRANSPARENT); // 文字背景透明
        wstring title = L"拼图小游戏";
        int tx = (winW - textwidth(title.c_str())) / 2; // 标题居中
        outtextxy(tx, 80, title.c_str()); // 在(tx,80)位置写标题

        // 2. 画蓝色的"开始游戏"按钮
        setfillcolor(RGB(100, 180, 255)); // 按钮颜色蓝色
        // 画圆角矩形按钮：左上角(btnStartX,btnStartY)，右下角(btnStartX+W,btnStartY+H)，圆角半径8
        solidroundrect(btnStartX, btnStartY, btnStartX + btnStartW, btnStartY + btnStartH, 8, 8);
        settextstyle(24, 0, L"微软雅黑");
        settextcolor(WHITE); // 按钮文字白色
        wstring startStr = L"开始游戏";
        // 按钮文字居中
        tx = btnStartX + (btnStartW - textwidth(startStr.c_str())) / 2;
        int ty = btnStartY + (btnStartH - textheight(startStr.c_str())) / 2;
        outtextxy(tx, ty, startStr.c_str());

        // 3. 画"选择难度："标题
        settextstyle(20, 0, L"微软雅黑");
        settextcolor(RGB(60, 60, 60));
        wstring diffTitle = L"选择难度：";
        outtextxy(winW / 2 - 160, 250, diffTitle.c_str());

        // 4. 画三个难度按钮
        wstring diffTexts[] = { L"简单 3×3", L"中等 4×4", L"困难 5×5" };
        // 循环画三个按钮（i=0画简单，i=1画中等，i=2画困难）
        for (int i = 0; i < 3; i++) {
            int x = (i == 0) ? btnDiff1X : (i == 1 ? btnDiff2X : btnDiff3X);
            // ✅ 核心：选中的难度按钮变成橙色，其他是灰色
            if (selectDiff == i + 1)
                setfillcolor(RGB(255, 200, 100)); // 橙色
            else
                setfillcolor(RGB(220, 220, 220)); // 灰色

            solidroundrect(x, btnDiff1Y, x + btnDiffW, btnDiff1Y + btnDiffH, 6, 6);
            settextstyle(16, 0, L"微软雅黑");
            settextcolor(BLACK);
            tx = x + (btnDiffW - textwidth(diffTexts[i].c_str())) / 2;
            ty = btnDiff1Y + (btnDiffH - textheight(diffTexts[i].c_str())) / 2;
            outtextxy(tx, ty, diffTexts[i].c_str());
        }
    }

    // 绘制游戏内状态栏（左上角步数，右上角时间）
    void drawGameStatus(int steps, wstring timeStr) const {
        settextstyle(24, 0, L"微软雅黑");
        settextcolor(RGB(40, 40, 40));
        setbkmode(TRANSPARENT);

        // 绘制左上角的步数
        wchar_t stepText[50];
        swprintf_s(stepText, L"步数：%d", steps);
        outtextxy(boardX, boardY - 40, stepText);

        // 绘制右上角的时间（右对齐）
        int tw = textwidth(timeStr.c_str());
        outtextxy(boardX + 360 - tw, boardY - 40, timeStr.c_str());
    }

    // 绘制游戏内按钮（原图预览、返回菜单）
    void drawGameButtons(bool showPreview) const {
        // 绘制预览按钮，显示预览时按钮变橙色
        setfillcolor(showPreview ? RGB(255, 200, 100) : RGB(200, 220, 240));
        solidroundrect(btnPreviewX, btnPreviewY, btnPreviewX + btnPreviewW, btnPreviewY + btnPreviewH, 5, 5);
        settextstyle(16, 0, L"微软雅黑");
        settextcolor(BLACK);
        setbkmode(TRANSPARENT);
        wstring str1 = showPreview ? L"关闭预览" : L"原图预览";
        int tx = btnPreviewX + (btnPreviewW - textwidth(str1.c_str())) / 2;
        int ty = btnPreviewY + (btnPreviewH - textheight(str1.c_str())) / 2;
        outtextxy(tx, ty, str1.c_str());

        // 绘制返回菜单按钮
        setfillcolor(RGB(240, 200, 200));
        solidroundrect(btnBackX, btnBackY, btnBackX + btnBackW, btnBackY + btnBackH, 5, 5);
        wstring str2 = L"返回菜单";
        tx = btnBackX + (btnBackW - textwidth(str2.c_str())) / 2;
        ty = btnBackY + (btnBackH - textheight(str2.c_str())) / 2;
        outtextxy(tx, ty, str2.c_str());
    }

    // 检测主菜单按钮点击，返回按钮编号
    int checkMenuClick(int mx, int my) const {
        // 点到了开始游戏按钮
        if (mx >= btnStartX && mx <= btnStartX + btnStartW &&
            my >= btnStartY && my <= btnStartY + btnStartH)
            return 1;
        // 点到了难度按钮
        if (my >= btnDiff1Y && my <= btnDiff1Y + btnDiffH) {
            if (mx >= btnDiff1X && mx <= btnDiff1X + btnDiffW) return 2; // 简单
            if (mx >= btnDiff2X && mx <= btnDiff2X + btnDiffW) return 3; // 中等
            if (mx >= btnDiff3X && mx <= btnDiff3X + btnDiffW) return 4; // 困难
        }
        return 0; // 没点到任何按钮
    }

    // 检测游戏内按钮点击，返回按钮编号
    int checkGameBtnClick(int mx, int my) const {
        if (mx >= btnPreviewX && mx <= btnPreviewX + btnPreviewW &&
            my >= btnPreviewY && my <= btnPreviewY + btnPreviewH)
            return 1; // 预览按钮
        if (mx >= btnBackX && mx <= btnBackX + btnBackW &&
            my >= btnBackY && my <= btnBackY + btnBackH)
            return 2; // 返回菜单按钮
        return 0;
    }

    // 获取棋盘左上角X坐标
    int getBoardX() const { return boardX; }
    // 获取棋盘左上角Y坐标
    int getBoardY() const { return boardY; }
};


// ===================== 6. 游戏主控类（总调度所有模块） =====================
// 作用：连接UI和逻辑，处理游戏流程，相当于游戏的大脑
class PuzzleGame {
private:
    Board board;         // 棋盘逻辑模块
    ImageManager imgMgr; // 图像管理模块（你负责）
    UIManager uiMgr;     // UI管理模块（你负责）
    GameTimer timer;     // 计时器模块

    int steps;           // 游戏步数
    bool gameOver;       // 游戏是否结束（通关）
    bool showPreview;    // 是否显示原图预览
    bool isPlaying;      // 是否正在游戏中（区分主菜单和游戏界面）
    int curDiff;         // 当前难度（1=简单，2=中等，3=困难）
    int curRows, curCols;// 当前棋盘行数和列数
    wstring curImgPath;  // 当前选中的图片路径

    // 根据难度更新棋盘行数和列数
    void updateDifficulty() {
        switch (curDiff) {
        case 1: curRows = 3; curCols = 3; break;
        case 2: curRows = 4; curCols = 4; break;
        case 3: curRows = 5; curCols = 5; break;
        default: curRows = 3; curCols = 3;
        }
    }

    // 通关后弹出提示框
    void showWinDialog() {
        wchar_t msg[200];
        // 格式化通关信息
        swprintf_s(msg, L"恭喜通关成功！\n\n总步数：%d\n用时：%s\n\n点击确定继续游戏",
            steps, timer.getTimeString().c_str());
        // 弹出Windows消息框
        MessageBoxW(GetHWnd(), msg, L"通关成功", MB_OK | MB_ICONINFORMATION);
    }

public:
    // 构造函数：初始化游戏状态
    PuzzleGame() : board(3, 3, 120), uiMgr(500, 520) {
        steps = 0;
        gameOver = false;
        showPreview = false;
        isPlaying = false;
        curDiff = 1;
        curRows = 3;
        curCols = 3;
        curImgPath = L"puzzle.jpg"; // 默认拼图图片路径
    }

    // 开始游戏
    void startGame() {
        updateDifficulty(); // 根据难度更新棋盘大小
        int pieceSize = 360 / curRows; // 计算单个拼图块大小（总棋盘360×360）
        board = Board(curRows, curCols, pieceSize); // 创建新棋盘
        board.init(); // 初始化棋盘
        board.shuffle(); // 打乱拼图
        imgMgr.loadImage(curImgPath, curRows, curCols, pieceSize); // 加载并切割图片

        // 重置游戏状态
        steps = 0;
        gameOver = false;
        showPreview = false;
        timer.start(); // 开始计时
        isPlaying = true; // 设置为游戏中状态
    }

    // 返回主菜单
    void backToMenu() {
        isPlaying = false;
        timer.reset();
    }

    // 处理鼠标点击事件
    void handleMouse(int x, int y) {
        // 如果在主菜单
        if (!isPlaying) {
            // 检测主菜单按钮点击
            int res = uiMgr.checkMenuClick(x, y);
            if (res == 1) startGame();          // 开始游戏
            else if (res >= 2 && res <= 4) curDiff = res - 1; // 选择难度
            return;
        }

        // 如果在游戏中，检测游戏内按钮点击
        int btn = uiMgr.checkGameBtnClick(x, y);
        if (btn == 1) { showPreview = !showPreview; return; } // 切换预览
        if (btn == 2) { backToMenu(); return; } // 返回菜单

        // 如果游戏已经结束，禁止点击棋盘
        if (gameOver) return;

        // 计算点击的棋盘坐标
        int bx = uiMgr.getBoardX();
        int by = uiMgr.getBoardY();
        int size = board.getPieceSize();
        int r = (y - by) / size; // 点击的行
        int c = (x - bx) / size; // 点击的列

        // 尝试移动拼图块
        if (board.clickPiece(r, c)) {
            steps++; // 步数加1
            // 检查是否通关
            if (board.isComplete()) {
                gameOver = true;
                timer.stop();    // 停止计时器，锁定最终时间
                render();        // 先刷新界面，显示完整拼图
                showWinDialog(); // 再弹出通关提示
            }
        }
    }

    // 渲染整个界面
    void render() {
        uiMgr.drawBackground(); // 先画背景（擦黑板）
        // 如果在主菜单
        if (!isPlaying) {
            uiMgr.drawMenu(curDiff);
        }
        // 如果在游戏中
        else {
            uiMgr.drawGameStatus(steps, timer.getTimeString()); // 画状态栏
            int bx = uiMgr.getBoardX();
            int by = uiMgr.getBoardY();
            int size = board.getPieceSize();

            // 如果显示预览，画完整原图
            if (showPreview) {
                imgMgr.drawOriginal(bx, by, 360, 360);
            }
            // 否则画拼图块
            else {
                // 遍历所有拼图块
                for (int r = 0; r < board.getRows(); r++)
                    for (int c = 0; c < board.getCols(); c++) {
                        const Piece& p = board.getPiece(r, c);
                        if (p.getIsBlank()) continue; // 跳过空白块
                        // 计算块的屏幕坐标，然后绘制
                        imgMgr.drawPiece(p.getId(), bx + c * size, by + r * size);
                    }
            }
            uiMgr.drawGameButtons(showPreview); // 画游戏内按钮
        }
        // ✅ 双缓冲关键：把内存里画好的图一次性刷新到屏幕，解决闪屏
        FlushBatchDraw();
    }

    // 游戏主循环（游戏的心脏，一直运行直到关闭窗口）
    void run() {
        initgraph(500, 520); // 创建500×520的图形窗口
        BeginBatchDraw();     // 开启双缓冲模式，所有绘制先在内存进行

        ExMessage msg; // 定义消息结构体，接收鼠标和键盘事件
        // 无限循环，直到用户关闭窗口
        while (true) {
            // 循环处理所有待处理的消息
            while (peekmessage(&msg, EM_MOUSE | EM_KEY)) {
                // 鼠标左键按下
                if (msg.message == WM_LBUTTONDOWN)
                    handleMouse(msg.x, msg.y);
                // ESC键按下，退出游戏
                if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {
                    EndBatchDraw();
                    closegraph();
                    return;
                }
            }
            render(); // 每帧渲染一次界面
            Sleep(10); // 休眠10毫秒，控制帧率约100FPS
        }
    }
};


// ===================== 程序入口（程序从这里开始运行） =====================
int main() {
    PuzzleGame game; // 创建游戏对象
    game.run();      // 启动游戏主循环
    return 0;
}
