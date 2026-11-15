#include <Arduino.h>
#include <TFT_eSPI.h>

#define JOY_X 34
#define JOY_Y 35
#define JOY_SW 32
#define TFT_BL 25

#define BTN_FIRE 13
#define BTN_EXIT 12
#define BTN_ENTER 14

enum Dir {NONE, UP, DOWN, LEFT, RIGHT};

struct Input {
  Dir d;
  bool fire;
  bool exitBtn;
  bool enterBtn;
  int analogX;
  int analogY;
};

Input readInput();
void text(int x, int y, const String& s, int font = 2, uint16_t fg = TFT_BLACK, uint16_t bg = TFT_WHITE);
void frame();
void title(const char* s);
void gameOver(const char* n, Input in);
void resetGame();
void hud(const char* n);

void riverRaid(Input in);
void dino(Input in);
void snake(Input in);
void tetris(Input in);
void breakout(Input in);
void flappy(Input in);
void pong(Input in);
void invaders(Input in);
void car(Input in);
void game2048(Input in);
void simon(Input in);
void maze(Input in);
void lander(Input in);
void mines(Input in);
void frogger(Input in);
void dodge(Input in);

const int W = 320, H = 240, DEAD = 650;
TFT_eSPI tft;
TFT_eSprite fb(&tft);

const char* apps[] = {
  "RIVER RAID (COLOR)", "Dino Runner", "Snake", "Tetris",
  "Breakout", "Flappy Bird", "Pong", "Space Invaders",
  "Car Dodger", "2048", "Simon Says", "Maze Escape",
  "Lunar Lander", "Minesweeper Mini", "Frogger", "Dodge Blocks"
};

int cx = 2048, cy = 2048, menu = 0, app = -1, score = 0;
uint32_t lastDir = 0, lastTick = 0, started = 0;

bool checkBtn(int pin, bool &lastState, uint32_t &lastTime) {
  bool reading = digitalRead(pin);
  uint32_t now = millis();
  if (reading != lastState && (now - lastTime) > 40) {
    lastTime = now;
    lastState = reading;
    if (reading == LOW) return true;
  }
  return false;
}

Input readInput() {
  Input z = {NONE, false, false, false, 0, 0};
  uint32_t n = millis();

  static bool stFire = HIGH, stExit = HIGH, stEnter = HIGH, stJoy = HIGH;
  static uint32_t tFire = 0, tExit = 0, tEnter = 0, tJoy = 0;

  bool pFire = checkBtn(BTN_FIRE, stFire, tFire);
  bool pExit = checkBtn(BTN_EXIT, stExit, tExit);
  bool pEnter = checkBtn(BTN_ENTER, stEnter, tEnter);
  bool pJoy = checkBtn(JOY_SW, stJoy, tJoy);

  z.fire = pFire || pJoy;
  z.exitBtn = pExit;
  z.enterBtn = pEnter || pJoy;

  int rawX = analogRead(JOY_X);
  int rawY = analogRead(JOY_Y);

  int diffX = rawX - cx;
  int diffY = rawY - cy;

  if (abs(diffY) > 250) z.analogX = constrain(map(diffY, -1800, 1800, -100, 100), -100, 100);
  if (abs(diffX) > 250) z.analogY = constrain(map(diffX, -1800, 1800, -100, 100), -100, 100);

  Dir d = NONE;
  if (rawX > cx + DEAD) d = DOWN;
  else if (rawX < cx - DEAD) d = UP;
  else if (rawY > cy + DEAD) d = RIGHT;
  else if (rawY < cy - DEAD) d = LEFT;

  if (d != NONE && (lastDir != d || n - lastTick >= (lastDir == NONE ? 240 : 120))) {
    z.d = d;
    lastDir = d;
    lastTick = n;
  }
  if (d == NONE) lastDir = NONE;

  return z;
}
void text(int x, int y, const String& s, int font, uint16_t fg, uint16_t bg) {
  fb.setTextFont(font);
  fb.setTextColor(fg, bg);
  fb.drawString(s, x, y);
}

void frame() {
  fb.pushSprite(0, 0);
}

void title(const char* s) {
  fb.fillSprite(TFT_WHITE);
  fb.drawRect(0, 0, W, H, TFT_BLACK);
  text(8, 6, s, 2, TFT_BLACK, TFT_WHITE);
  fb.drawFastHLine(0, 25, W, TFT_BLACK);
}

void gameOver(const char* n, Input in) {
  fb.fillSprite(TFT_NAVY);
  fb.drawRect(0, 0, W, H, TFT_WHITE);
  text(8, 6, n, 2, TFT_YELLOW, TFT_NAVY);
  text(85, 65, "GAME OVER", 4, TFT_RED, TFT_NAVY);
  text(105, 115, "Score: " + String(score), 2, TFT_WHITE, TFT_NAVY);
  text(45, 175, "ENTER: Zagraj ponownie", 2, TFT_GREEN, TFT_NAVY);
  text(65, 200, "EXIT: Powrot do menu", 2, TFT_CYAN, TFT_NAVY);
  frame();

  if (in.exitBtn) app = -1;
  else if (in.enterBtn) resetGame();
}

int px = 150, py = 200, vx = 2, vy = -2, ax = 0, ay = 0, life = 3, level = 1;
bool over = false, paused = false;
int snakeX[80], snakeY[80], snakeN = 4, foodX = 12, foodY = 8;
Dir snakeD = RIGHT;
int board[4][4];
int tet[20][10];
int carLane = 1;
int cursorX = 0, cursorY = 0;

float rrPlaneX = 160.0;
float rrFuel = 100.0;
int rrBulletX = -1, rrBulletY = -1;
int rrBankL[25], rrBankR[25];
int rrTargetBankL = 60, rrTargetBankR = 260;
int rrScrollCounter = 0;
struct RREnemy { bool active; int type; float x, y; int dir; };
RREnemy rrObjs[6];

void initRiverRaid() {
  rrPlaneX = 160.0;
  rrFuel = 100.0;
  rrBulletX = -1;
  rrBulletY = -1;
  rrTargetBankL = 60;
  rrTargetBankR = 260;
  rrScrollCounter = 0;
  for (int i = 0; i < 25; i++) {
    rrBankL[i] = 60;
    rrBankR[i] = 260;
  }
  for (int i = 0; i < 6; i++) rrObjs[i].active = false;
}

void resetGame() {
  score = 0; life = 3; level = 1; over = false; paused = false;
  px = 150; py = 190; vx = 2; vy = -2; ax = 0; ay = 0;
  started = millis();
  snakeN = 4; snakeD = RIGHT;
  for (int i = 0; i < snakeN; i++) { snakeX[i] = 8 - i; snakeY[i] = 7; }
  foodX = random(2, 28); foodY = random(2, 19);
  memset(board, 0, sizeof(board));
  board[0][0] = 2; board[1][1] = 2;
  memset(tet, 0, sizeof(tet));
  carLane = 1; cursorX = cursorY = 0;
  initRiverRaid();
}

void hud(const char* n) {
  text(5, 28, n, 2);
  text(230, 28, "S:" + String(score), 2);
  text(5, 212, "L:" + String(life) + " Lv:" + String(level), 2);
}

void riverRaid(Input in) {
  int speed = 3;
  if (in.analogY < -30) speed = 5;
  else if (in.analogY > 30) speed = 2;

  if (abs(in.analogX) > 15) {
    rrPlaneX += (in.analogX / 100.0) * 4.5;
  }
  rrPlaneX = constrain(rrPlaneX, 15, W - 15);

  if (in.fire && rrBulletY < 0) {
    rrBulletX = (int)rrPlaneX;
    rrBulletY = 185;
  }

  if (rrBulletY >= 0) {
    rrBulletY -= 10;
    if (rrBulletY < 26) rrBulletY = -1;
  }

  rrScrollCounter += speed;
  if (rrScrollCounter >= 10) {
    rrScrollCounter = 0;
    for (int i = 23; i > 0; i--) {
      rrBankL[i] = rrBankL[i - 1];
      rrBankR[i] = rrBankR[i - 1];
    }
    if (random(100) < 25) {
      int width = random(110, 190);
      rrTargetBankL = random(20, W - width - 20);
      rrTargetBankR = rrTargetBankL + width;
    }
    if (rrBankL[0] < rrTargetBankL) rrBankL[0] += 4;
    else if (rrBankL[0] > rrTargetBankL) rrBankL[0] -= 4;
    if (rrBankR[0] < rrTargetBankR) rrBankR[0] += 4;
    else if (rrBankR[0] > rrTargetBankR) rrBankR[0] -= 4;

    if (random(100) < 18) {
      for (int i = 0; i < 6; i++) {
        if (!rrObjs[i].active) {
          rrObjs[i].active = true;
          int r = random(100);
          if (r < 25) rrObjs[i].type = 2;
          else if (r < 60) rrObjs[i].type = 0;
          else rrObjs[i].type = 1;
          rrObjs[i].x = random(rrBankL[0] + 20, rrBankR[0] - 20);
          rrObjs[i].y = 26;
          rrObjs[i].dir = (random(2) == 0) ? 1 : -1;
          break;
        }
      }
    }
  }

  rrFuel -= 0.08 * (speed / 3.0);
  if (rrFuel <= 0) {
    rrFuel = 0;
    over = true;
  }

  fb.fillSprite(TFT_BLUE);
  for (int i = 0; i < 22; i++) {
    int yPos = 24 + i * 10;
    fb.fillRect(0, yPos, rrBankL[i], 10, TFT_DARKGREEN);
    fb.fillRect(rrBankR[i], yPos, W - rrBankR[i], 10, TFT_DARKGREEN);
  }

  if ((int)rrPlaneX - 8 <= rrBankL[17] || (int)rrPlaneX + 8 >= rrBankR[17]) {
    over = true;
  }

  for (int i = 0; i < 6; i++) {
    if (!rrObjs[i].active) continue;
    rrObjs[i].y += speed;

    if (rrObjs[i].type == 0 || rrObjs[i].type == 1) {
      float objSpd = (rrObjs[i].type == 1) ? 2.2 : 1.2;
      rrObjs[i].x += rrObjs[i].dir * objSpd;
      int rowIdx = constrain(((int)rrObjs[i].y - 24) / 10, 0, 21);
      if (rrObjs[i].x - 14 <= rrBankL[rowIdx]) rrObjs[i].dir = 1;
      if (rrObjs[i].x + 14 >= rrBankR[rowIdx]) rrObjs[i].dir = -1;
    }

    int ox = (int)rrObjs[i].x;
    int oy = (int)rrObjs[i].y;

    if (rrObjs[i].type == 0) {
      fb.fillRect(ox - 14, oy - 4, 28, 8, TFT_MAROON);
      fb.fillRect(ox - 6, oy - 8, 12, 4, TFT_LIGHTGREY);
    } else if (rrObjs[i].type == 1) {
      fb.fillCircle(ox, oy, 6, TFT_ORANGE);
      fb.drawFastHLine(ox - 10, oy - 7, 20, TFT_WHITE);
      fb.fillRect(ox - 12, oy - 2, 6, 4, TFT_YELLOW);
    } else if (rrObjs[i].type == 2) {
      fb.fillRect(ox - 10, oy - 12, 20, 24, TFT_RED);
      fb.fillRect(ox - 10, oy - 4, 20, 8, TFT_WHITE);
      text(ox - 7, oy - 4, "F", 1, TFT_BLACK, TFT_WHITE);
    }

    if (rrBulletY >= 0 && abs(rrBulletX - ox) < 16 && abs(rrBulletY - oy) < 12) {
      rrObjs[i].active = false;
      rrBulletY = -1;
      score += (rrObjs[i].type == 2) ? 80 : 50;
      continue;
    }

    if (abs((int)rrPlaneX - ox) < 16 && abs(195 - oy) < 14) {
      if (rrObjs[i].type == 2) {
        rrFuel = min(100.0f, rrFuel + 1.8f);
      } else {
        over = true;
      }
    }

    if (oy > H + 15) rrObjs[i].active = false;
  }

  if (rrBulletY >= 0) {
    fb.fillRect(rrBulletX - 1, rrBulletY, 3, 8, TFT_YELLOW);
  }

  int pxInt = (int)rrPlaneX;
  fb.fillTriangle(pxInt, 183, pxInt - 4, 205, pxInt + 4, 205, TFT_WHITE);
  fb.fillTriangle(pxInt, 191, pxInt - 13, 201, pxInt + 13, 201, TFT_LIGHTGREY);
  fb.fillTriangle(pxInt, 201, pxInt - 6, 207, pxInt + 6, 207, TFT_RED);

  fb.fillRect(0, 0, W, 24, TFT_BLACK);
  text(6, 4, "SCORE: " + String(score), 2, TFT_YELLOW, TFT_BLACK);
  text(155, 4, "FUEL", 2, TFT_WHITE, TFT_BLACK);
  fb.drawRect(205, 4, 104, 16, TFT_WHITE);
  uint16_t fuelColor = (rrFuel > 30) ? TFT_GREEN : TFT_RED;
  fb.fillRect(207, 6, (int)rrFuel, 12, fuelColor);
}
void dino(Input in) {
  title("DINO RUNNER");
  static int ox = 300;
  if ((in.fire || in.d == UP) && py >= 190) vy = -10;
  vy += 1; py += vy;
  if (py > 190) { py = 190; vy = 0; }
  ox -= 4 + score / 80;
  if (ox < -15) { ox = 330; score++; }
  fb.fillRect(20, py, 14, 20, TFT_BLACK);
  fb.fillRect(ox, 190, 10, 20, TFT_BLACK);
  fb.drawFastHLine(0, 210, W, TFT_BLACK);
  if (ox < 34 && ox + 10 > 20 && py + 20 > 190) over = true;
  hud("FIRE/UP: Skok");
}

void snake(Input in) {
  title("SNAKE");
  if (in.fire) paused = !paused;
  if (in.d != NONE && !((snakeD == LEFT && in.d == RIGHT) || (snakeD == RIGHT && in.d == LEFT) || (snakeD == UP && in.d == DOWN) || (snakeD == DOWN && in.d == UP))) snakeD = in.d;
  if (!paused && millis() - lastTick > 120) {
    lastTick = millis();
    for (int i = snakeN - 1; i; i--) { snakeX[i] = snakeX[i - 1]; snakeY[i] = snakeY[i - 1]; }
    if (snakeD == LEFT) snakeX[0]--;
    if (snakeD == RIGHT) snakeX[0]++;
    if (snakeD == UP) snakeY[0]--;
    if (snakeD == DOWN) snakeY[0]++;
    if (snakeX[0] < 0 || snakeX[0] >= 30 || snakeY[0] < 0 || snakeY[0] >= 18) over = true;
    for (int i = 1; i < snakeN; i++) if (snakeX[i] == snakeX[0] && snakeY[i] == snakeY[0]) over = true;
    if (snakeX[0] == foodX && snakeY[0] == foodY) {
      if (snakeN < 80) snakeN++;
      score += 10; foodX = random(30); foodY = random(18);
    }
  }
  for (int i = 0; i < snakeN; i++) fb.fillRect(10 + snakeX[i] * 10, 35 + snakeY[i] * 9, 8, 7, TFT_DARKGREEN);
  fb.fillRect(10 + foodX * 10, 35 + foodY * 9, 8, 7, TFT_RED);
  hud(paused ? "PAUZA" : "FIRE: Pauza");
}

void tetris(Input in) {
  title("TETRIS");
  static int tx = 4, ty = 0;
  if (in.d == LEFT) tx = max(0, tx - 1);
  if (in.d == RIGHT) tx = min(8, tx + 1);
  if (in.d == DOWN) ty++;
  if (in.d == UP) ty = 19;
  if (in.fire) score += 1;
  if (millis() - lastTick > max(100, 700 - score * 5)) { lastTick = millis(); ty++; }
  if (ty > 19) { tet[19][tx] = 1; ty = 0; tx = 4; score += 10; }
  for (int y = 0; y < 20; y++) for (int x = 0; x < 10; x++) if (tet[y][x]) fb.fillRect(100 + x * 12, 35 + y * 8, 11, 7, TFT_BLUE);
  fb.fillRect(100 + tx * 12, 35 + ty * 8, 11, 7, TFT_RED);
  hud("UP: Opusc");
}

void breakout(Input in) {
  title("BREAKOUT");
  static bool run = false;
  if (in.d == LEFT) px -= 10;
  if (in.d == RIGHT) px += 10;
  if (in.fire) run = true;
  px = constrain(px, 10, 270);
  if (run) {
    ax += vx; ay += vy;
    if (ax < 0 || ax > 315) vx = -vx;
    if (ay < 35) { vy = -vy; score += 5; }
    if (ay > 220) {
      if (ax >= px && ax <= px + 45) vy = -abs(vy);
      else { life--; ax = 160; ay = 150; if (!life) over = true; }
    }
  }
  for (int r = 0; r < 3; r++) for (int c = 0; c < 10; c++) fb.fillRect(10 + c * 30, 45 + r * 12, 27, 9, TFT_ORANGE);
  fb.fillRect(px, 215, 45, 5, TFT_NAVY);
  fb.fillCircle(ax, ay, 3, TFT_RED);
  hud("FIRE: Start pilki");
}

void flappy(Input in) {
  title("FLAPPY BIRD");
  static int pipe = 320, gap = 100;
  if (in.fire || in.d == UP) vy = -6;
  vy++; py += vy; pipe -= 3;
  if (pipe < -25) { pipe = 320; gap = random(65, 150); score++; }
  fb.fillCircle(80, py, 7, TFT_ORANGE);
  fb.fillRect(pipe, 35, 22, gap - 35, TFT_DARKGREEN);
  fb.fillRect(pipe, gap + 55, 22, 210 - gap, TFT_DARKGREEN);
  if (py < 35 || py > 210 || (80 > pipe && 80 < pipe + 22 && (py < gap || py > gap + 55))) over = true;
  hud("FIRE/UP: Lot");
}

void pong(Input in) {
  title("PONG");
  static bool run = false;
  static int opp = 100, ps = 0, os = 0;
  if (in.d == UP) py -= 8;
  if (in.d == DOWN) py += 8;
  if (in.fire) run = true;
  py = constrain(py, 35, 190);
  opp += (ay > opp ? 2 : -2);
  if (run) {
    ax += vx; ay += vy;
    if (ay < 35 || ay > 210) vy = -vy;
    if (ax < 12 && ay > py && ay < py + 38) vx = abs(vx);
    if (ax > 300 && ay > opp && ay < opp + 38) vx = -abs(vx);
    if (ax < 0) { os++; ax = 160; ay = 120; }
    if (ax > 320) { ps++; ax = 160; ay = 120; }
    if (ps >= 5 || os >= 5) over = true;
  }
  fb.fillRect(8, py, 4, 38, TFT_BLUE);
  fb.fillRect(308, opp, 4, 38, TFT_RED);
  fb.fillCircle(ax, ay, 3, TFT_BLACK);
  text(140, 45, String(ps) + " : " + String(os), 2);
  hud("FIRE: Serw");
}

void invaders(Input in) {
  title("SPACE INVADERS");
  static int ex = 30, bulletY = -1;
  if (in.d == LEFT) px -= 7;
  if (in.d == RIGHT) px += 7;
  if (in.fire && bulletY < 0) bulletY = 190;
  px = constrain(px, 10, 300);
  ex += (millis() / 500 % 2 ? 1 : -1);
  if (bulletY >= 0) {
    bulletY -= 8;
    if (bulletY < 40) { bulletY = -1; score += 10; }
  }
  for (int r = 0; r < 3; r++) for (int c = 0; c < 8; c++) fb.fillRect(ex + c * 30, 50 + r * 18, 12, 8, TFT_PURPLE);
  fb.fillTriangle(px, 205, px + 12, 205, px + 6, 195, TFT_DARKGREEN);
  if (bulletY >= 0) fb.drawFastVLine(px + 6, bulletY, 8, TFT_RED);
  hud("FIRE: Strzal");
}

void car(Input in) {
  title("CAR DODGER");
  if (in.d == LEFT) carLane = max(0, carLane - 1);
  if (in.d == RIGHT) carLane = min(2, carLane + 1);
  int y = 40 + (millis() / 8 % (170));
  fb.fillRect(70 + carLane * 60, 190, 25, 35, TFT_BLUE);
  fb.fillRect(70 + ((millis() / 700) % 3) * 60, y, 25, 35, TFT_RED);
  if (y > 170 && ((millis() / 700) % 3) == carLane) over = true;
  score = millis() / 250;
  hud("LEFT/RIGHT: Pasy");
}
void game2048(Input in) {
  title("2048");
  if (in.d != NONE) {
    int a = random(4), b = random(4);
    board[a][b] += 2;
    score += board[a][b];
  }
  for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) {
    fb.drawRect(55 + x * 54, 40 + y * 40, 50, 36, TFT_BLACK);
    if (board[y][x]) text(64 + x * 54, 50 + y * 40, String(board[y][x]), 2);
  }
  hud("4 kierunki");
}

void simon(Input in) {
  title("SIMON SAYS");
  static Dir seq[20];
  static int n = 1, pos = 0;
  static uint32_t show = 0;
  if (!show) { seq[0] = (Dir)random(1, 5); show = millis(); }
  if (millis() - show < 800) {
    text(125, 100, "PATRZ", 4, TFT_RED, TFT_WHITE);
    text(145, 145, String(seq[(millis() / 250) % n]), 4);
  } else if (in.d != NONE) {
    if (in.d == seq[pos]) {
      if (++pos == n) { n++; pos = 0; seq[n - 1] = (Dir)random(1, 5); show = millis(); score = n - 1; }
    } else over = true;
  }
  hud("Powtorz kierunki");
}

void maze(Input in) {
  title("MAZE ESCAPE");
  static int mx = 1, my = 1;
  if (in.fire) { mx = 1; my = 1; }
  if (in.d == RIGHT) mx++;
  if (in.d == LEFT) mx--;
  if (in.d == UP) my--;
  if (in.d == DOWN) my++;
  mx = constrain(mx, 1, 18); my = constrain(my, 1, 11);
  for (int y = 0; y < 13; y++) for (int x = 0; x < 20; x++) {
    if (x == 0 || y == 0 || x == 19 || y == 12 || ((x * 7 + y * 3) % 9 == 0)) fb.fillRect(30 + x * 13, 40 + y * 13, 12, 12, TFT_BLACK);
  }
  fb.fillRect(30 + mx * 13, 40 + my * 13, 10, 10, TFT_RED);
  if (mx == 18 && my == 11) { level++; mx = 1; my = 1; score += 50; }
  hud("FIRE: Reset");
}

void lander(Input in) {
  title("LUNAR LANDER");
  if (in.d == LEFT) vx--;
  if (in.d == RIGHT) vx++;
  if (in.fire || in.d == UP) { vy -= 2; score = max(0, score - 1); }
  vy += 0.15; px += vx; py += vy;
  fb.drawFastHLine(120, 210, 60, TFT_DARKGREEN);
  fb.fillTriangle(px, py, px + 12, py, px + 6, py - 12, TFT_BLUE);
  if (py >= 210) {
    if (abs(vx) < 3 && vy < 3) { score += 100; level++; py = 50; vx = vy = 0; }
    else over = true;
  }
  hud(("Paliwo:" + String(100 - score)).c_str());
}

void mines(Input in) {
  title("MINESWEEPER MINI");
  if (in.d == LEFT) cursorX = max(0, cursorX - 1);
  if (in.d == RIGHT) cursorX = min(9, cursorX + 1);
  if (in.d == UP) cursorY = max(0, cursorY - 1);
  if (in.d == DOWN) cursorY = min(7, cursorY + 1);
  for (int y = 0; y < 8; y++) for (int x = 0; x < 10; x++) fb.drawRect(45 + x * 23, 40 + y * 20, 20, 17, TFT_BLACK);
  fb.drawRect(45 + cursorX * 23, 40 + cursorY * 20, 20, 17, TFT_RED);
  if (in.fire) score++;
  hud("FIRE: Odkryj");
}

void frogger(Input in) {
  title("FROGGER");
  if (in.d == LEFT) px -= 15;
  if (in.d == RIGHT) px += 15;
  if (in.d == UP) py -= 15;
  if (in.d == DOWN) py += 15;
  px = constrain(px, 0, 305); py = constrain(py, 35, 210);
  for (int r = 0; r < 5; r++) fb.fillRect((millis() / 8 + r * 70) % 320, 55 + r * 28, 55, 12, TFT_MAROON);
  fb.fillRect(px, py, 12, 12, TFT_DARKGREEN);
  if (py < 45) { score += 100; level++; py = 205; }
  hud("Dojdz na gore");
}

void dodge(Input in) {
  title("DODGE BLOCKS");
  if (in.d == LEFT) px -= 8;
  if (in.d == RIGHT) px += 8;
  if (in.d == UP) py -= 8;
  if (in.d == DOWN) py += 8;
  px = constrain(px, 0, 310); py = constrain(py, 35, 210);
  int bx = (millis() / 5) % 320, by = (millis() / 7) % 210 + 30;
  fb.fillRect(px, py, 10, 10, TFT_BLUE);
  fb.fillRect(bx, by, 14, 14, TFT_RED);
  if (abs(bx - px) < 12 && abs(by - py) < 12) over = true;
  score = (millis() - started) / 100;
  hud("Unikaj blokow");
}

void drawMenu() {
  fb.fillSprite(TFT_NAVY);
  fb.drawRect(0, 0, W, H, TFT_WHITE);
  text(18, 6, "KONSOLA GIER RETRO", 2, TFT_YELLOW, TFT_NAVY);
  fb.drawFastHLine(0, 26, W, TFT_WHITE);

  int first = constrain(menu - 1, 0, 12);
  for (int i = 0; i < 4; i++) {
    int n = first + i;
    if (n == menu) {
      fb.fillRect(8, 38 + i * 38, 304, 32, TFT_CYAN);
      text(18, 46 + i * 38, String(n + 1) + ". " + apps[n], 2, TFT_BLACK, TFT_CYAN);
    } else {
      fb.drawRect(8, 38 + i * 38, 304, 32, TFT_WHITE);
      text(18, 46 + i * 38, String(n + 1) + ". " + apps[n], 2, TFT_WHITE, TFT_NAVY);
    }
  }

  text(8, 198, first ? "^ wiecej" : "", 2, TFT_LIGHTGREY, TFT_NAVY);
  text(235, 198, first < 12 ? "v wiecej" : "", 2, TFT_LIGHTGREY, TFT_NAVY);
  text(25, 218, "GORA/DOL: Wybor   ENTER: Start", 2, TFT_GREEN, TFT_NAVY);
  frame();
}

void runGame(Input in) {
  if (in.exitBtn) {
    app = -1;
    return;
  }

  if (over) {
    gameOver(apps[app], in);
    return;
  }

  switch (app) {
    case 0: riverRaid(in); break;
    case 1: dino(in); break;
    case 2: snake(in); break;
    case 3: tetris(in); break;
    case 4: breakout(in); break;
    case 5: flappy(in); break;
    case 6: pong(in); break;
    case 7: invaders(in); break;
    case 8: car(in); break;
    case 9: game2048(in); break;
    case 10: simon(in); break;
    case 11: maze(in); break;
    case 12: lander(in); break;
    case 13: mines(in); break;
    case 14: frogger(in); break;
    case 15: dodge(in); break;
  }
  if (app >= 0) frame();
}

void setup() {
  Serial.begin(115200);
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BTN_FIRE, INPUT_PULLUP);
  pinMode(BTN_EXIT, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  analogReadResolution(12);
  analogSetPinAttenuation(JOY_X, ADC_11db);
  analogSetPinAttenuation(JOY_Y, ADC_11db);

  long sx = 0, sy = 0;
  for (int i = 0; i < 160; i++) {
    sx += analogRead(JOY_X);
    sy += analogRead(JOY_Y);
    delay(2);
  }
  cx = sx / 160;
  cy = sy / 160;

  tft.init();
  tft.setRotation(3);
  fb.setColorDepth(8);
  fb.createSprite(W, H);
  randomSeed(esp_random());
}

void loop() {
  Input in = readInput();
  if (app < 0) {
    if (in.d == UP) menu = max(0, menu - 1);
    if (in.d == DOWN) menu = min(15, menu + 1);
    if (in.enterBtn) {
      app = menu;
      resetGame();
    }
    drawMenu();
  } else {
    runGame(in);
  }
  delay(12);
}
