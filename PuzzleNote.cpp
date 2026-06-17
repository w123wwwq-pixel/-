// ===================== 引入所需头文件 =====================
#include <iostream>      // 标准输入输出流（cout/cin）
#include <vector>        // 动态数组容器，用来存拼图块、图片等
#include <string>        // 字符串处理
#include <ctime>         // 时间函数，用于随机数种子和计时
#include <graphics.h>    // EasyX图形库，所有绘图功能
#include <windows.h>     // Windows系统API，用于获取文件路径、弹窗等
#include <commdlg.h>     // 通用对话框库，用于打开文件选择框

// 链接comdlg32库，否则文件选择框无法运行
#pragma comment(lib, "comdlg32.lib")

// 使用标准命名空间，不用每次写std::
using namespace std;

// ===================== 全局工具函数：获取exe所在绝对目录 =====================
// 作用：解决图片路径问题，让程序在任何位置都能找到images文件夹里的图片
wstring GetExeDir() {
    // 定义一个字符数组，用来存exe的完整路径
    wchar_t path[MAX_PATH] = { 0 };
    // 调用Windows API获取当前运行程序的完整路径
    GetModuleFileNameW(NULL, path, MAX_PATH);
    // 把字符数组转成wstring宽字符串（Windows中文用宽字符串）
    wstring strPath(path);
    // 找到路径中最后一个斜杠的位置（区分文件名和目录）
    size_t pos = strPath.find_last_of(L"\\/");
    // 如果找到斜杠，截取斜杠前面的部分（就是exe所在目录）
    if (pos != wstring::npos) {
        return strPath.substr(0, pos);
    }
    // 如果没找到，返回当前目录
    return L".";
}

// ===================== 1. 拼图块实体类 =====================
// 作用：纯数据类，只保存单个拼图块的信息，没有业务逻辑
class Piece {
private:
    // 拼图块的唯一编号（0-8对应3×3拼图，0-15对应4×4）
    int id;
    // 拼图块当前所在的行
    int row;
    // 拼图块当前所在的列
    int col;
    // 标记是否是空白块（最后一个块是空白块）
    bool isBlank;

public:
    // 无参构造函数，默认初始化
    Piece() : id(0), row(0), col(0), isBlank(false) {}

    // 有参构造函数，创建拼图块时直接赋值
    Piece(int id, int r, int c, bool blank = false)
        : id(id), row(r), col(c), isBlank(blank) {
    }

    // 获取拼图块编号
    int getId() const { return id; }
    // 获取拼图块当前行
    int getRow() const { return row; }
    // 获取拼图块当前列
    int getCol() const { return col; }
    // 判断是否是空白块
    bool getIsBlank() const { return isBlank; }

    // 设置拼图块所在行
    void setRow(int r) { row = r; }
    // 设置拼图块所在列
    void setCol(int c) { col = c; }
};

// ===================== 2. 计时器类 =====================
// 作用：专门负责游戏计时，独立于其他模块，可复用
class GameTimer {
private:
    // 游戏开始时的时间戳（毫秒）
    clock_t startTime;
    // 计时器是否正在运行
    bool running;
    // 停止计时时保存的最终用时（秒）
    int savedSeconds;

public:
    // 构造函数，初始化计时器状态
    GameTimer() : startTime(0), running(false), savedSeconds(0) {}

    // 开始计时
    void start() {
        // 记录当前时间作为开始时间
        startTime = clock();
        // 设置计时器为运行状态
        running = true;
        // 重置保存的时间
        savedSeconds = 0;
    }

    // 停止计时，锁定当前最终时间（通关时调用）
    void stop() {
        // 只有正在运行时才停止
        if (running) {
            // 计算从开始到现在的秒数，保存起来
            savedSeconds = (int)(clock() - startTime) / CLOCKS_PER_SEC;
            // 设置计时器为停止状态
            running = false;
        }
    }

    // 重置计时器
    void reset() {
        startTime = 0;
        running = false;
        savedSeconds = 0;
    }

    // 获取当前已用时间（秒）
    int getSeconds() const {
        // 如果正在运行，实时计算时间
        if (running) {
            return (int)(clock() - startTime) / CLOCKS_PER_SEC;
        }
        // 如果已经停止，返回之前保存的最终时间
        return savedSeconds;
    }

    // 获取格式化的时间字符串（MM:SS格式）
    wstring getTimeString() const {
        int sec = getSeconds();
        // 计算分钟和秒
        int min = sec / 60;
        sec %= 60;
        // 定义字符数组存格式化后的字符串
        wchar_t buf[20];
        // 格式化输出，不足两位补0
        swprintf_s(buf, L"%02d:%02d", min, sec);
        // 转成wstring返回
        return wstring(buf);
    }
};

// ===================== 3. 棋盘核心逻辑类 =====================
// 作用：处理所有游戏规则：拼图移动、打乱、通关判断
class Board {
private:
    // 棋盘行数
    int rows;
    // 棋盘列数
    int cols;
    // 单个拼图块的大小（像素）
    int pieceSize;
    // 存储所有拼图块的动态数组
    vector<Piece> pieces;
    // 空白块当前所在行
    int blankRow;
    // 空白块当前所在列
    int blankCol;

    // 私有方法：交换两个拼图块的位置
    void swapPieces(int r1, int c1, int r2, int c2) {
        // 计算两个块在数组中的索引
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
        // 初始化棋盘
        init();
    }

    // 初始化棋盘：生成有序的拼图块
    void init() {
        // 清空之前的拼图块
        pieces.clear();
        // 遍历所有行和列
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // 计算块的编号
                int id = i * cols + j;
                // 最后一个块是空白块
                bool blank = (i == rows - 1 && j == cols - 1);
                // 创建拼图块并加入数组
                pieces.emplace_back(id, i, j, blank);
            }
        }
        // 初始化空白块在右下角
        blankRow = rows - 1;
        blankCol = cols - 1;
    }

    // 打乱拼图：通过随机移动空白块实现，天然保证可解
    void shuffle() {
        // 设置随机数种子为当前时间，保证每次打乱都不一样
        srand((unsigned)time(NULL));
        // 随机移动2000次，足够打乱
        for (int i = 0; i < 2000; i++) {
            // 随机生成一个行和列
            int r = rand() % rows;
            int c = rand() % cols;
            // 尝试点击这个块，能移动就移动
            clickPiece(r, c);
        }
    }

    // 点击拼图块，尝试移动
    bool clickPiece(int r, int c) {
        // 检查行和列是否在合法范围内
        if (r < 0 || r >= rows || c < 0 || c >= cols) return false;
        // 判断点击的块是否和空白块相邻（上下左右）
        bool adjacent = (abs(r - blankRow) == 1 && c == blankCol) ||
            (abs(c - blankCol) == 1 && r == blankRow);
        // 如果相邻，交换两个块的位置
        if (adjacent) {
            swapPieces(r, c, blankRow, blankCol);
            return true;
        }
        // 不相邻，无法移动
        return false;
    }

    // 判断是否通关：所有拼图块都在正确位置
    bool isComplete() const {
        // 遍历所有拼图块
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++) {
                int idx = i * cols + j;
                // 跳过空白块
                if (pieces[idx].getIsBlank()) continue;
                // 如果块的编号和它所在位置的索引不一致，说明没通关
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
    // 存储完整的原始图片
    IMAGE originalImg;
    // 存储切割后的所有拼图块图片
    vector<IMAGE> pieceImgs;
    // 拼图行数
    int rows;
    // 拼图列数
    int cols;
    // 单个拼图块大小
    int pieceSize;

    // 私有方法：生成默认的彩色数字拼图块（图片加载失败时调用）
    void generateDefault() {
        // 清空之前的拼图块图片
        pieceImgs.clear();
        // 定义9种不同的颜色，用于默认块
        COLORREF colors[] = {
            RGB(255, 180, 180), RGB(180, 255, 180), RGB(180, 180, 255),
            RGB(255, 255, 180), RGB(255, 180, 255), RGB(180, 255, 255),
            RGB(255, 220, 180), RGB(220, 180, 255), RGB(180, 220, 255)
        };

        // 生成所有拼图块
        for (int i = 0; i < rows * cols; i++) {
            // 创建一个指定大小的空白图片
            IMAGE block(pieceSize, pieceSize);
            // 设置当前绘图目标为这个空白图片（在内存里画，不显示在屏幕）
            SetWorkingImage(&block);

            // 填充背景色，用i取模9循环使用颜色
            setfillcolor(colors[i % 9]);
            solidrectangle(0, 0, pieceSize, pieceSize);
            // 绘制边框
            setlinecolor(RGB(60, 60, 60));
            rectangle(0, 0, pieceSize - 1, pieceSize - 1);

            // 绘制块上的数字（i+1，从1开始显示）
            wchar_t num[10];
            swprintf_s(num, L"%d", i + 1);
            settextstyle(pieceSize / 2, 0, L"微软雅黑");
            settextcolor(BLACK);
            setbkmode(TRANSPARENT); // 文字背景透明
            // 计算文字居中坐标
            int tx = (pieceSize - textwidth(num)) / 2;
            int ty = (pieceSize - textheight(num)) / 2;
            outtextxy(tx, ty, num);

            // 恢复绘图目标为屏幕
            SetWorkingImage(NULL);
            // 把画好的块加入数组
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

        // 计算完整拼图的总宽度和高度
        int totalW = cols * pieceSize;
        int totalH = rows * pieceSize;

        // 调用EasyX API加载图片，自动缩放到指定大小
        int ret = loadimage(&originalImg, path.c_str(), totalW, totalH, true);
        // 如果加载失败（文件不存在、格式错误），生成默认数字块
        if (ret != 0) {
            generateDefault();
            return;
        }

        // 设置绘图目标为原始图片，开始切割
        SetWorkingImage(&originalImg);
        // 遍历所有行和列
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                // 创建一个空白块
                IMAGE block(pieceSize, pieceSize);
                // 从原始图片的指定位置截取一块，保存到block中
                getimage(&block, j * pieceSize, i * pieceSize, pieceSize, pieceSize);

                // 给每个块加边框
                SetWorkingImage(&block);
                setlinecolor(RGB(80, 80, 80));
                rectangle(0, 0, pieceSize - 1, pieceSize - 1);
                // 切回原始图片继续切割下一块
                SetWorkingImage(&originalImg);

                // 把切割好的块加入数组
                pieceImgs.push_back(block);
            }
        }
        // 恢复绘图目标为屏幕
        SetWorkingImage(NULL);
    }

    // 绘制单个拼图块到指定坐标
    void drawPiece(int id, int x, int y) const {
        // 检查id是否合法
        if (id >= 0 && id < (int)pieceImgs.size())
            // 调用EasyX API绘制图片
            putimage(x, y, &pieceImgs[id]);
    }

    // 绘制完整的原始图片（原图预览功能）
    void drawOriginal(int x, int y, int w, int h) const {
        putimage(x, y, w, h, &originalImg, 0, 0);
    }
};

// ===================== 5. UI管理类（✅ 你负责的模块） =====================
// 作用：所有界面绘制和按钮点击检测，不包含任何游戏逻辑
class UIManager {
private:
    // 窗口宽度
    int winW;
    // 窗口高度
    int winH;
    // 棋盘左上角X坐标
    int boardX;
    // 棋盘左上角Y坐标
    int boardY;

    // 主菜单按钮坐标和尺寸
    int btnStartX, btnStartY, btnStartW, btnStartH;
    int btnSelectImgX, btnSelectImgY, btnSelectImgW, btnSelectImgH;
    int btnDiff1X, btnDiff1Y, btnDiffW, btnDiffH;
    int btnDiff2X, btnDiff2Y;
    int btnDiff3X, btnDiff3Y;

    // 游戏内按钮坐标和尺寸
    int btnPreviewX, btnPreviewY, btnPreviewW, btnPreviewH;
    int btnBackX, btnBackY, btnBackW, btnBackH;

public:
    // 构造函数：初始化所有界面元素的坐标
    UIManager(int w, int h) : winW(w), winH(h) {
        boardX = 70;
        boardY = 80;

        // 主菜单按钮坐标计算（居中对齐）
        btnStartW = 200; btnStartH = 50;
        btnStartX = (winW - btnStartW) / 2;
        btnStartY = 180;

        btnSelectImgW = 200; btnSelectImgH = 40;
        btnSelectImgX = (winW - btnSelectImgW) / 2;
        btnSelectImgY = 250;

        btnDiffW = 100; btnDiffH = 40;
        btnDiff1X = winW / 2 - 160;
        btnDiff2X = winW / 2 - 50;
        btnDiff3X = winW / 2 + 60;
        btnDiff1Y = btnDiff2Y = btnDiff3Y = 330;

        // 游戏内按钮坐标
        btnPreviewW = 120; btnPreviewH = 35;
        btnBackW = 120; btnBackH = 35;
        btnPreviewX = boardX;
        btnBackX = boardX + 360 - btnBackW;
        btnPreviewY = btnBackY = boardY + 360 + 20;
    }

    // 绘制背景
    void drawBackground() const {
        // 设置背景色为浅灰色
        setbkcolor(RGB(245, 245, 245));
        // 清屏，用背景色填充整个窗口
        cleardevice();
    }

    // 绘制主菜单界面
    void drawMenu(int selectDiff, wstring curImgName) const {
        // 绘制标题
        settextstyle(48, 0, L"微软雅黑");
        settextcolor(RGB(50, 50, 80));
        setbkmode(TRANSPARENT);
        wstring title = L"拼图小游戏";
        // 标题居中
        int tx = (winW - textwidth(title.c_str())) / 2;
        outtextxy(tx, 80, title.c_str());

        // 绘制开始游戏按钮
        setfillcolor(RGB(100, 180, 255));
        // 绘制圆角矩形按钮
        solidroundrect(btnStartX, btnStartY, btnStartX + btnStartW, btnStartY + btnStartH, 8, 8);
        settextstyle(24, 0, L"微软雅黑");
        settextcolor(WHITE);
        wstring startStr = L"开始游戏";
        // 按钮文字居中
        tx = btnStartX + (btnStartW - textwidth(startStr.c_str())) / 2;
        int ty = btnStartY + (btnStartH - textheight(startStr.c_str())) / 2;
        outtextxy(tx, ty, startStr.c_str());

        // 绘制选择图片按钮
        setfillcolor(RGB(180, 220, 180));
        solidroundrect(btnSelectImgX, btnSelectImgY, btnSelectImgX + btnSelectImgW, btnSelectImgY + btnSelectImgH, 6, 6);
        settextstyle(18, 0, L"微软雅黑");
        settextcolor(BLACK);
        wstring imgStr = L"选择拼图图片";
        tx = btnSelectImgX + (btnSelectImgW - textwidth(imgStr.c_str())) / 2;
        ty = btnSelectImgY + (btnSelectImgH - textheight(imgStr.c_str())) / 2;
        outtextxy(tx, ty, imgStr.c_str());

        // 显示当前选中的图片名称
        settextstyle(14, 0, L"微软雅黑");
        settextcolor(RGB(100, 100, 100));
        wstring tip = L"当前图片：" + curImgName;
        tx = (winW - textwidth(tip.c_str())) / 2;
        outtextxy(tx, btnSelectImgY + btnSelectImgH + 10, tip.c_str());

        // 绘制难度选择标题
        settextstyle(20, 0, L"微软雅黑");
        settextcolor(RGB(60, 60, 60));
        wstring diffTitle = L"选择难度：";
        outtextxy(winW / 2 - 160, 300, diffTitle.c_str());

        // 绘制三个难度按钮
        wstring diffTexts[] = { L"简单 3×3", L"中等 4×4", L"困难 5×5" };
        for (int i = 0; i < 3; i++) {
            int x = (i == 0) ? btnDiff1X : (i == 1 ? btnDiff2X : btnDiff3X);
            // 选中的难度按钮用橙色高亮
            if (selectDiff == i + 1)
                setfillcolor(RGB(255, 200, 100));
            else
                setfillcolor(RGB(220, 220, 220));

            solidroundrect(x, btnDiff1Y, x + btnDiffW, btnDiff1Y + btnDiffH, 6, 6);
            settextstyle(16, 0, L"微软雅黑");
            settextcolor(BLACK);
            tx = x + (btnDiffW - textwidth(diffTexts[i].c_str())) / 2;
            ty = btnDiff1Y + (btnDiffH - textheight(diffTexts[i].c_str())) / 2;
            outtextxy(tx, ty, diffTexts[i].c_str());
        }
    }

    // 绘制游戏内状态栏（步数和时间）
    void drawGameStatus(int steps, wstring timeStr) const {
        settextstyle(24, 0, L"微软雅黑");
        settextcolor(RGB(40, 40, 40));
        setbkmode(TRANSPARENT);

        // 绘制步数
        wchar_t stepText[50];
        swprintf_s(stepText, L"步数：%d", steps);
        outtextxy(boardX, boardY - 40, stepText);

        // 绘制时间（右对齐）
        int tw = textwidth(timeStr.c_str());
        outtextxy(boardX + 360 - tw, boardY - 40, timeStr.c_str());
    }

    // 绘制游戏内按钮
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
        // 开始游戏按钮
        if (mx >= btnStartX && mx <= btnStartX + btnStartW &&
            my >= btnStartY && my <= btnStartY + btnStartH)
            return 1;
        // 选择图片按钮
        if (mx >= btnSelectImgX && mx <= btnSelectImgX + btnSelectImgW &&
            my >= btnSelectImgY && my <= btnSelectImgY + btnSelectImgH)
            return 5;
        // 难度选择按钮
        if (my >= btnDiff1Y && my <= btnDiff1Y + btnDiffH) {
            if (mx >= btnDiff1X && mx <= btnDiff1X + btnDiffW) return 2;
            if (mx >= btnDiff2X && mx <= btnDiff2X + btnDiffW) return 3;
            if (mx >= btnDiff3X && mx <= btnDiff3X + btnDiffW) return 4;
        }
        // 没有点击任何按钮
        return 0;
    }

    // 检测游戏内按钮点击，返回按钮编号
    int checkGameBtnClick(int mx, int my) const {
        // 预览按钮
        if (mx >= btnPreviewX && mx <= btnPreviewX + btnPreviewW &&
            my >= btnPreviewY && my <= btnPreviewY + btnPreviewH)
            return 1;
        // 返回菜单按钮
        if (mx >= btnBackX && mx <= btnBackX + btnBackW &&
            my >= btnBackY && my <= btnBackY + btnBackH)
            return 2;
        return 0;
    }

    // 获取棋盘左上角X坐标
    int getBoardX() const { return boardX; }
    // 获取棋盘左上角Y坐标
    int getBoardY() const { return boardY; }
};

// ===================== 6. 游戏主控类 =====================
// 作用：总调度所有模块，处理游戏流程，连接UI和逻辑
class PuzzleGame {
private:
    // 棋盘逻辑模块
    Board board;
    // 图像管理模块（你负责）
    ImageManager imgMgr;
    // UI管理模块（你负责）
    UIManager uiMgr;
    // 计时器模块
    GameTimer timer;

    // 游戏步数
    int steps;
    // 游戏是否结束（通关）
    bool gameOver;
    // 是否显示原图预览
    bool showPreview;
    // 是否正在游戏中（区分主菜单和游戏界面）
    bool isPlaying;
    // 当前难度（1-简单，2-中等，3-困难）
    int curDiff;
    // 当前棋盘行数
    int curRows;
    // 当前棋盘列数
    int curCols;
    // 当前选中的图片路径
    wstring curImgPath;

    // 从完整路径中提取文件名（显示在主菜单）
    wstring getImgFileName() const {
        size_t pos = curImgPath.find_last_of(L"\\/");
        if (pos == wstring::npos) return curImgPath;
        return curImgPath.substr(pos + 1);
    }

    // 打开文件选择对话框，选择拼图图片
    bool openImageDialog() {
        // 定义字符数组存选中的文件路径
        wchar_t filePath[MAX_PATH] = { 0 };
        // 初始化对话框结构体
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        // 对话框父窗口为游戏窗口
        ofn.hwndOwner = GetHWnd();
        // 过滤文件类型，只显示图片
        ofn.lpstrFilter = L"图片文件 (*.jpg;*.jpeg;*.png)\0*.jpg;*.jpeg;*.png\0所有文件 (*.*)\0*.*\0";
        ofn.lpstrFile = filePath;
        ofn.nMaxFile = MAX_PATH;
        // 对话框标题
        ofn.lpstrTitle = L"选择一张拼图图片";
        // 标志：文件必须存在、路径必须存在
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        // 设置对话框默认打开目录为exe同级的images文件夹
        wstring initDir = GetExeDir() + L"\\images";
        ofn.lpstrInitialDir = initDir.c_str();

        // 打开对话框，用户选择文件后返回true
        if (GetOpenFileNameW(&ofn)) {
            curImgPath = filePath;
            return true;
        }
        // 用户取消选择
        return false;
    }

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
        // 默认图片路径
        curImgPath = L"puzzle.jpg";
    }

    // 开始游戏
    void startGame() {
        // 根据难度更新棋盘大小
        updateDifficulty();
        // 计算单个拼图块大小（总棋盘360×360）
        int pieceSize = 360 / curRows;
        // 创建新的棋盘
        board = Board(curRows, curCols, pieceSize);
        // 初始化棋盘
        board.init();
        // 打乱拼图
        board.shuffle();
        // 加载并切割图片
        imgMgr.loadImage(curImgPath, curRows, curCols, pieceSize);

        // 重置游戏状态
        steps = 0;
        gameOver = false;
        showPreview = false;
        // 开始计时
        timer.start();
        // 设置为游戏中状态
        isPlaying = true;
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
            else if (res == 5) openImageDialog(); // 选择图片
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
        // 先绘制背景（清屏）
        uiMgr.drawBackground();
        // 如果在主菜单
        if (!isPlaying) {
            uiMgr.drawMenu(curDiff, getImgFileName());
        }
        // 如果在游戏中
        else {
            // 绘制状态栏
            uiMgr.drawGameStatus(steps, timer.getTimeString());
            int bx = uiMgr.getBoardX();
            int by = uiMgr.getBoardY();
            int size = board.getPieceSize();

            // 如果显示预览，绘制完整原图
            if (showPreview) {
                imgMgr.drawOriginal(bx, by, 360, 360);
            }
            // 否则绘制拼图块
            else {
                // 遍历所有拼图块
                for (int r = 0; r < board.getRows(); r++)
                    for (int c = 0; c < board.getCols(); c++) {
                        const Piece& p = board.getPiece(r, c);
                        // 跳过空白块
                        if (p.getIsBlank()) continue;
                        // 计算块的屏幕坐标
                        imgMgr.drawPiece(p.getId(), bx + c * size, by + r * size);
                    }
            }
            // 绘制游戏内按钮
            uiMgr.drawGameButtons(showPreview);
        }
        // ✅ 双缓冲关键：把内存里画好的图一次性刷新到屏幕，解决闪屏
        FlushBatchDraw();
    }

    // 游戏主循环
    void run() {
        // 创建500×520的图形窗口
        initgraph(500, 520);
        // ✅ 开启双缓冲模式，所有绘制先在内存进行
        BeginBatchDraw();

        // 定义消息结构体，接收鼠标和键盘事件
        ExMessage msg;
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
            // 每帧渲染一次界面
            render();
            // 休眠10毫秒，控制帧率约100FPS
            Sleep(10);
        }
    }
};

// ===================== 程序入口 =====================
int main() {
    // 创建游戏对象
    PuzzleGame game;
    // 启动游戏主循环
    game.run();
    return 0;
}
