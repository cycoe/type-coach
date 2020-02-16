#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <wchar.h>

int main(int argc, char *argv[])
{
  wchar_t wc; //定义一个宽字符变量，及一个宽字符数组。

  // 设置 locale
  setlocale(LC_ALL, "zh_CN.utf8");
  initscr();
  noecho();
  cbreak();
  refresh();

  //// 读取一个字符
  // wscanf(L"%lc", &wc); //输入值。
  //// 打印字符到屏幕，%lc 很重要，否则无法输出中文，编译时需链接 -lncursesw
  // printw("输出是: %lc\n", wc);
  //// 刷新屏幕
  // refresh();

  // check if cursor changes before refresh
  // wchar_t line[10] = {L"你好hello"};
  // unsigned offset[10] = {0};
  // unsigned cursor = 0;
  // for (int i = 0; i < 10; i++) {
  // printw("%lc", line[i]);
  // offset[i] = getcurx(stdscr) - cursor;
  // cursor = getcurx(stdscr);
  //}
  //
  // for (int i = 0; i < 10; i++) {
  // printf("%d ", offset[i]);
  //}

  attron(A_REVERSE);
  mvprintw(1, 2, "你");
  attroff(A_REVERSE);
  refresh();
  getch();
  mvprintw(1, 1, "好");
  refresh();

  getch();
  endwin();
  return 0;
}
