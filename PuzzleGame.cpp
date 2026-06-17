#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <graphics.h>
#include <windows.h>
using namespace std;

// ===================== 1. 拼图块实体类 =====================
class Piece {
private:
    int id;
    int row;
    int col;
    bool isBlank;
public:
    Piece() : id(0), row(0), col(0), isBlank(false) {}
    Piece(int id, int r, int c, bool blank = false)
        : id(id), row(r), col(c), isBlank(blank) {
    }

    int getId() const { return id; }
    int getRow() const { return row; }
    int getCol() const { return col; }
    bool getIsBlank() const { return isBlank; }

    void setRow(int r) { row = r; }
    void setCol(int c) { col = c; }
};

// ===================== 2. 计时器类 =====================
class GameTimer {
private:
    clock_t startTime;
    bool running;
    int savedSeconds; // 保存停止时的最终用时

public:
    GameTimer() : startTime(0), running(false), savedSeconds(0) {}

    void start() {
        startTime = clock();
        running = true;
        savedSeconds = 0;
    }

    // 停止计时，锁定当前最终时间
    void stop() {
        if (running) {
            savedSeconds = (int)(clock() - startTime) / CLOCKS_PER_SEC;
            running = false;
        }
    }

    void reset() {
        startTime = 0;
        running = false;
        savedSeconds = 0;
    }

    int getSeconds() const {
        if (running) {
            return (int)(clock() - startTime) / CLOCKS_PER_SEC;
        }
        return savedSeconds; // 停止后返回锁定的最终时间
    }

    wstring getTimeString() const {
        int sec = getSeconds();
        int min = sec / 60;
        sec %= 60;
        wchar_t buf[20];
        swprintf_s(buf, L"%02d:%02d", min, sec);
        return wstring(buf);
    }
};

// ===================== 3. 棋盘核心逻辑类 =====================
class Board {
private:
    int rows;
    int cols;
    int pieceSize;
    vector<Piece> pieces;
    int blankRow, blankCol;

    void swapPieces(int r1, int c1, int r2, int c2) {
        int idx1 = r1 * cols + c1;
        int idx2 = r2 * cols + c2;
        swap(pieces[idx1], pieces[idx2]);
        blankRow = r1;
        blankCol = c1;
    }

public:
    Board(int r, int c, int size) : rows(r), cols(c), pieceSize(size) { init(); }

    void init() {
        pieces.clear();
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                int id = i * cols + j;
                bool blank = (i == rows - 1 && j == cols - 1);
                pieces.emplace_back(id, i, j, blank);
            }
        }
        blankRow = rows - 1;
        blankCol = cols - 1;
    }

    void shuffle() {
        srand((unsigned)time(NULL));
        for (int i = 0; i < 2000; i++) {
            int r = rand() % rows;
            int c = rand() % cols;
            clickPiece(r, c);
        }
    }

    bool clickPiece(int r, int c) {
        if (r < 0 || r >= rows || c < 0 || c >= cols) return false;
        bool adjacent = (abs(r - blankRow) == 1 && c == blankCol) ||
            (abs(c - blankCol) == 1 && r == blankRow);
        if (adjacent) {
            swapPieces(r, c, blankRow, blankCol);
            return true;
        }
        return false;
    }

    bool isComplete() const {
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++) {
                int idx = i * cols + j;
                if (pieces[idx].getIsBlank()) continue;
                if (pieces[idx].getId() != idx) return false;
            }
        return true;
    }

    const Piece& getPiece(int r, int c) const { return pieces[r * cols + c]; }
    int getRows() const { return rows; }
    int getCols() const { return cols; }
    int getPieceSize() const { return pieceSize; }
};

// ===================== 4. 图像管理类 =====================
class ImageManager {
private:
    IMAGE originalImg;
    vector<IMAGE> pieceImgs;
    int rows, cols, pieceSize;

    void generateDefault() {
        pieceImgs.clear();
        COLORREF colors[] = {
            RGB(255, 180, 180), RGB(180, 255, 180), RGB(180, 180, 255),
            RGB(255, 255, 180), RGB(255, 180, 255), RGB(180, 255, 255),
            RGB(255, 220, 180), RGB(220, 180, 255), RGB(180, 220, 255)
        };

        for (int i = 0; i < rows * cols; i++) {
            IMAGE block(pieceSize, pieceSize);
            SetWorkingImage(&block);

            setfillcolor(colors[i % 9]);
            solidrectangle(0, 0, pieceSize, pieceSize);
            setlinecolor(RGB(60, 60, 60));
            rectangle(0, 0, pieceSize - 1, pieceSize - 1);

            wchar_t num[10];
            swprintf_s(num, L"%d", i + 1);
            settextstyle(pieceSize / 2, 0, L"微软雅黑");
            settextcolor(BLACK);
            setbkmode(TRANSPARENT);
            int tx = (pieceSize - textwidth(num)) / 2;
            int ty = (pieceSize - textheight(num)) / 2;
            outtextxy(tx, ty, num);

            SetWorkingImage(NULL);
            pieceImgs.push_back(block);
        }
    }

public:
    ImageManager() : rows(0), cols(0), pieceSize(0) {}

    void loadImage(wstring path, int r, int c, int size) {
        rows = r; cols = c; pieceSize = size;
        pieceImgs.clear();

        int totalW = cols * pieceSize;
        int totalH = rows * pieceSize;

        int ret = loadimage(&originalImg, path.c_str(), totalW, totalH, true);
        if (ret != 0) {
            generateDefault();
            return;
        }

        SetWorkingImage(&originalImg);
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                IMAGE block(pieceSize, pieceSize);
                getimage(&block, j * pieceSize, i * pieceSize, pieceSize, pieceSize);

                SetWorkingImage(&block);
                setlinecolor(RGB(80, 80, 80));
                rectangle(0, 0, pieceSize - 1, pieceSize - 1);
                SetWorkingImage(&originalImg);

                pieceImgs.push_back(block);
            }
        }
        SetWorkingImage(NULL);
    }

    void drawPiece(int id, int x, int y) const {
        if (id >= 0 && id < (int)pieceImgs.size())
            putimage(x, y, &pieceImgs[id]);
    }

    void drawOriginal(int x, int y, int w, int h) const {
        putimage(x, y, w, h, &originalImg, 0, 0);
    }
};

// ===================== 5. UI管理类 =====================
class UIManager {
private:
    int winW, winH;
    int boardX, boardY;
    // 主菜单按钮
    int btnStartX, btnStartY, btnStartW, btnStartH;
    int btnDiff1X, btnDiff1Y, btnDiffW, btnDiffH;
    int btnDiff2X, btnDiff2Y;
    int btnDiff3X, btnDiff3Y;
    // 游戏内按钮
    int btnPreviewX, btnPreviewY, btnPreviewW, btnPreviewH;
    int btnBackX, btnBackY, btnBackW, btnBackH;

public:
    UIManager(int w, int h) : winW(w), winH(h) {
        boardX = 70;
        boardY = 80;

        // 主菜单开始按钮坐标
        btnStartW = 200; btnStartH = 50;
        btnStartX = (winW - btnStartW) / 2;
        btnStartY = 180;

        // 难度按钮坐标（已上移优化布局）
        btnDiffW = 100; btnDiffH = 40;
        btnDiff1X = winW / 2 - 160;
        btnDiff2X = winW / 2 - 50;
        btnDiff3X = winW / 2 + 60;
        btnDiff1Y = btnDiff2Y = btnDiff3Y = 280;

        // 游戏内按钮坐标
        btnPreviewW = 120; btnPreviewH = 35;
        btnBackW = 120; btnBackH = 35;
        btnPreviewX = boardX;
        btnBackX = boardX + 360 - btnBackW;
        btnPreviewY = btnBackY = boardY + 360 + 20;
    }

    void drawBackground() const {
        setbkcolor(RGB(245, 245, 245));
        cleardevice();
    }

    // 绘制主菜单
    void drawMenu(int selectDiff) const {
        // 绘制标题
        settextstyle(48, 0, L"微软雅黑");
        settextcolor(RGB(50, 50, 80));
        setbkmode(TRANSPARENT);
        wstring title = L"拼图小游戏";
        int tx = (winW - textwidth(title.c_str())) / 2;
        outtextxy(tx, 80, title.c_str());

        // 绘制开始游戏按钮
        setfillcolor(RGB(100, 180, 255));
        solidroundrect(btnStartX, btnStartY, btnStartX + btnStartW, btnStartY + btnStartH, 8, 8);
        settextstyle(24, 0, L"微软雅黑");
        settextcolor(WHITE);
        wstring startStr = L"开始游戏";
        tx = btnStartX + (btnStartW - textwidth(startStr.c_str())) / 2;
        int ty = btnStartY + (btnStartH - textheight(startStr.c_str())) / 2;
        outtextxy(tx, ty, startStr.c_str());

        // 难度选择标题
        settextstyle(20, 0, L"微软雅黑");
        settextcolor(RGB(60, 60, 60));
        wstring diffTitle = L"选择难度：";
        outtextxy(winW / 2 - 160, 250, diffTitle.c_str());

        // 绘制三个难度按钮
        wstring diffTexts[] = { L"简单 3×3", L"中等 4×4", L"困难 5×5" };
        for (int i = 0; i < 3; i++) {
            int x = (i == 0) ? btnDiff1X : (i == 1 ? btnDiff2X : btnDiff3X);
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

    // 绘制游戏内状态栏
    void drawGameStatus(int steps, wstring timeStr) const {
        settextstyle(24, 0, L"微软雅黑");
        settextcolor(RGB(40, 40, 40));
        setbkmode(TRANSPARENT);

        wchar_t stepText[50];
        swprintf_s(stepText, L"步数：%d", steps);
        outtextxy(boardX, boardY - 40, stepText);

        int tw = textwidth(timeStr.c_str());
        outtextxy(boardX + 360 - tw, boardY - 40, timeStr.c_str());
    }

    // 绘制游戏内按钮
    void drawGameButtons(bool showPreview) const {
        setfillcolor(showPreview ? RGB(255, 200, 100) : RGB(200, 220, 240));
        solidroundrect(btnPreviewX, btnPreviewY, btnPreviewX + btnPreviewW, btnPreviewY + btnPreviewH, 5, 5);
        settextstyle(16, 0, L"微软雅黑");
        settextcolor(BLACK);
        setbkmode(TRANSPARENT);
        wstring str1 = showPreview ? L"关闭预览" : L"原图预览";
        int tx = btnPreviewX + (btnPreviewW - textwidth(str1.c_str())) / 2;
        int ty = btnPreviewY + (btnPreviewH - textheight(str1.c_str())) / 2;
        outtextxy(tx, ty, str1.c_str());

        setfillcolor(RGB(240, 200, 200));
        solidroundrect(btnBackX, btnBackY, btnBackX + btnBackW, btnBackY + btnBackH, 5, 5);
        wstring str2 = L"返回菜单";
        tx = btnBackX + (btnBackW - textwidth(str2.c_str())) / 2;
        ty = btnBackY + (btnBackH - textheight(str2.c_str())) / 2;
        outtextxy(tx, ty, str2.c_str());
    }

    // 主菜单点击检测
    int checkMenuClick(int mx, int my) const {
        if (mx >= btnStartX && mx <= btnStartX + btnStartW &&
            my >= btnStartY && my <= btnStartY + btnStartH)
            return 1;
        if (my >= btnDiff1Y && my <= btnDiff1Y + btnDiffH) {
            if (mx >= btnDiff1X && mx <= btnDiff1X + btnDiffW) return 2;
            if (mx >= btnDiff2X && mx <= btnDiff2X + btnDiffW) return 3;
            if (mx >= btnDiff3X && mx <= btnDiff3X + btnDiffW) return 4;
        }
        return 0;
    }

    // 游戏内按钮检测
    int checkGameBtnClick(int mx, int my) const {
        if (mx >= btnPreviewX && mx <= btnPreviewX + btnPreviewW &&
            my >= btnPreviewY && my <= btnPreviewY + btnPreviewH)
            return 1;
        if (mx >= btnBackX && mx <= btnBackX + btnBackW &&
            my >= btnBackY && my <= btnBackY + btnBackH)
            return 2;
        return 0;
    }

    int getBoardX() const { return boardX; }
    int getBoardY() const { return boardY; }
};

// ===================== 6. 游戏主控类 =====================
class PuzzleGame {
private:
    Board board;
    ImageManager imgMgr;
    UIManager uiMgr;
    GameTimer timer;

    int steps;
    bool gameOver;
    bool showPreview;
    bool isPlaying;
    int curDiff;
    int curRows, curCols;
    wstring curImgPath;

    void updateDifficulty() {
        switch (curDiff) {
        case 1: curRows = 3; curCols = 3; break;
        case 2: curRows = 4; curCols = 4; break;
        case 3: curRows = 5; curCols = 5; break;
        default: curRows = 3; curCols = 3;
        }
    }

    // 通关弹窗
    void showWinDialog() {
        wchar_t msg[200];
        swprintf_s(msg, L"恭喜通关成功！\n\n总步数：%d\n用时：%s\n\n点击确定继续游戏",
            steps, timer.getTimeString().c_str());
        MessageBoxW(GetHWnd(), msg, L"通关成功", MB_OK | MB_ICONINFORMATION);
    }

public:
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

    void startGame() {
        updateDifficulty();
        int pieceSize = 360 / curRows;
        board = Board(curRows, curCols, pieceSize);
        board.init();
        board.shuffle();
        imgMgr.loadImage(curImgPath, curRows, curCols, pieceSize);

        steps = 0;
        gameOver = false;
        showPreview = false;
        timer.start();
        isPlaying = true;
    }

    void backToMenu() {
        isPlaying = false;
        timer.reset();
    }

    void handleMouse(int x, int y) {
        if (!isPlaying) {
            int res = uiMgr.checkMenuClick(x, y);
            if (res == 1) startGame();
            else if (res >= 2 && res <= 4) curDiff = res - 1;
            return;
        }

        int btn = uiMgr.checkGameBtnClick(x, y);
        if (btn == 1) { showPreview = !showPreview; return; }
        if (btn == 2) { backToMenu(); return; }

        if (gameOver) return;

        int bx = uiMgr.getBoardX();
        int by = uiMgr.getBoardY();
        int size = board.getPieceSize();
        int r = (y - by) / size;
        int c = (x - bx) / size;

        if (board.clickPiece(r, c)) {
            steps++;
            if (board.isComplete()) {
                gameOver = true;
                timer.stop();    // 停止计时器，锁定最终通关时间
                render();        // 先刷新完整拼图画面
                showWinDialog(); // 再弹出通关提示
            }
        }
    }

    void render() {
        uiMgr.drawBackground();
        if (!isPlaying) {
            uiMgr.drawMenu(curDiff);
        }
        else {
            uiMgr.drawGameStatus(steps, timer.getTimeString());
            int bx = uiMgr.getBoardX();
            int by = uiMgr.getBoardY();
            int size = board.getPieceSize();

            if (showPreview) {
                imgMgr.drawOriginal(bx, by, 360, 360);
            }
            else {
                for (int r = 0; r < board.getRows(); r++)
                    for (int c = 0; c < board.getCols(); c++) {
                        const Piece& p = board.getPiece(r, c);
                        if (p.getIsBlank()) continue;
                        imgMgr.drawPiece(p.getId(), bx + c * size, by + r * size);
                    }
            }
            uiMgr.drawGameButtons(showPreview);
        }
        FlushBatchDraw();
    }

    void run() {
        initgraph(500, 520);
        BeginBatchDraw();

        ExMessage msg;
        while (true) {
            while (peekmessage(&msg, EM_MOUSE | EM_KEY)) {
                if (msg.message == WM_LBUTTONDOWN)
                    handleMouse(msg.x, msg.y);
                if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {
                    EndBatchDraw();
                    closegraph();
                    return;
                }
            }
            render();
            Sleep(10);
        }
    }
};

// ===================== 程序入口 =====================
int main() {
    PuzzleGame game;
    game.run();
    return 0;
}
