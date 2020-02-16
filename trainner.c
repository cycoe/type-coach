#include "utils.h"
#include "windows.c"
#include <locale.h>
#include <malloc.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef struct SIZE {
  unsigned height;
  unsigned width;
} SIZE;

typedef struct POS {
  int x;
  int y;
} POS;

#define MALLOC_INIT_SIZE 10
#define MALLOC_INC_STEP 10
#define LINESIZ 4096

#define WWIDTH 80
#define WHEIGHT 40

SIZE main_size = {40, 60};
SIZE panel_size = {40, 20};
SIZE popup_size = {20, 40};
SIZE hint_size = {3, 10};

wchar_t **read_article(const char *article_file, unsigned *num_line) {
  /* 存储单字节字符串的缓冲区 */
  char sbuf[LINESIZ] = {0};
  /* 存储多字节字符串的缓冲区 */
  wchar_t wbuf[LINESIZ] = {0};
  /* 待返回指针 */
  wchar_t **lines, *line;
  /* nstore: 已存储的字符数量，nw：转换出来的字符数量，noffset：wbuf
   * 中已被使用的字符数 */
  size_t nw, nstore, noffset, nlen;
  size_t nline = 0, nmalloc = MALLOC_INIT_SIZE;

  lines = (wchar_t **)calloc(sizeof(wchar_t *), nmalloc);

  FILE *fp = fopen(article_file, "r");
  if (fp == NULL)
    return NULL;

  nstore = 0;
  nlen = 0;
  line = (wchar_t *)calloc(sizeof(wchar_t), main_size.width - 3);
  while (fgets(sbuf, LINESIZ, fp)) {
    nw = mbstowcs(wbuf, sbuf, LINESIZ);
    noffset = 0;
    while (noffset < nw) {
      /* skip returns */
      if (wbuf[noffset] == '\n') {
        noffset++;
        continue;
      }
      line[nstore] = wbuf[noffset++];
      if (line[nstore] < 256) {
        nlen += 1;
      } else {
        nlen += 2;
      }
      nstore++;
      if (nlen == main_size.width - 4 ||
          (nlen == main_size.width - 5 && wbuf[noffset] > 256)) {
        lines[nline++] = line;
        if (nline == nmalloc) {
          nmalloc += MALLOC_INC_STEP;
          lines = realloc(lines, sizeof(wchar_t *) * nmalloc);
        }
        nstore = 0;
        nlen = 0;
        line = (wchar_t *)calloc(sizeof(wchar_t), main_size.width - 3);
      }
    }
  }
  lines[nline++] = line;
  lines = realloc(lines, sizeof(wchar_t *) * nline);

  *num_line = nline;
  return lines;
}

/* free the memory of lines */
void destroy_lines(wchar_t **lines, unsigned num_line) {
  for (int i = 0; i < num_line; i++)
    free(lines[i]);
  free(lines);
}

/* draw the referring lines */
int draw_refer_lines(WINDOW *win, wchar_t **lines, unsigned nline, char **xchar,
                     unsigned ntop, unsigned ndisplay) {
  wchar_t *line;

  for (unsigned y = ntop; y < ntop + ndisplay; y++) {
    wattron(win, A_UNDERLINE);
    line = lines[y];
    wmove(win, 3 * (y - ntop) + 3, 2);
    for (unsigned x = 0; line[x]; x++) {
      xchar[y][x] = getcurx(win);
      wprintw(win, "%lc", line[x]);
    }
    wattroff(win, A_UNDERLINE);
  }

  return 1;
}

int draw_input_lines(WINDOW *win, wchar_t **lines, unsigned nline,
                     Node **mistakes, char **xchar, POS *parray, unsigned ntop,
                     unsigned ndisplay) {
  wchar_t temp;
  Node *mistake;

  /* 储存当前光标所在位置字符，并将当前光标所在位置置为 0 */
  temp = lines[parray->y][parray->x];
  lines[parray->y][parray->x] = 0;

  for (unsigned y = ntop; y <= parray->y && y < ntop + ndisplay; y++) {
    /* 打印屏幕上显示的第一行到当前行的原始文本 */
    mvwprintw(win, 3 * (y - ntop) + 4, 2, "%ls", lines[y]);

    /* 打印错误文本 */
    wattron(win, A_REVERSE);
    for (mistake = mistakes[y]; mistake; mistake = mistake->next) {
      wmove(win, (y - ntop) * 3 + 4, xchar[y][mistake->offset]);
      if (mistake->ch > 255 && lines[y][mistake->offset] < 256) {
        wprintw(win, "x");
      } else if (mistake->ch < 256 && lines[y][mistake->offset] > 255) {
        wprintw(win, "%lc ", mistake->ch);
      } else {
        wprintw(win, "%lc", mistake->ch);
      }
    }
    wattroff(win, A_REVERSE);
  }

  /* 恢复当前光标的字符 */
  lines[parray->y][parray->x] = temp;

  /* 设置光标位置 */
  wmove(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x]);

  return 1;
}

int input_ch(WINDOW *win, wchar_t **lines, unsigned nline, unsigned ntop,
             Node **mistakes, char **xchar, POS *parray, wchar_t input) {

  int if_new_line = 0;

  /* 如果输入字符与参考字符不匹配 */
  if (input != lines[parray->y][parray->x]) {
    /* 将该字符压入错误栈 */
    push_table(mistakes + parray->y, parray->x, input);
    /* 开启反色模式 */
    wattron(win, A_REVERSE);
  }

  /* 分三种情况打印字符 */
  wmove(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x]);
  if (input > 255 && lines[parray->y][parray->x] < 256) {
    wprintw(win, "x");
  } else if (input < 256 && lines[parray->y][parray->x] > 255) {
    wprintw(win, "%lc ", input);
  } else {
    wprintw(win, "%lc", input);
  }
  wattroff(win, A_REVERSE);
  wrefresh(win);

  /* 处理游标在 lines 中的位置 */
  parray->x++;
  /* 如果到达行末尾，换行 */
  while (lines[parray->y][parray->x] == 0) {
    parray->y++;
    parray->x = 0;
    if_new_line = 1;
  }

  /* 如果文件到达末尾，返回 -1
   * 发生换行返回 1，没有换行返回 0
   */
  if (parray->y >= nline) {
    return -1;
  } else {
    return if_new_line;
  }
}

int delete_ch(WINDOW *win, wchar_t **lines, unsigned nline, unsigned ntop,
              Node **mistakes, char **xchar, POS *parray) {

  int if_new_line = 0;

  --parray->x;
  while (parray->x < 0 && parray->y > 0) {
    parray->y--;
    parray->x = wstrlen(lines[parray->y]) - 1;
    if_new_line = 1;
  }

  /* 到达文件开头返回 -1 */
  if (parray->x < 0) {
    parray->x = 0;
    return -1;
  }

  /* 删除当前光标后面的错误记录 */
  while (mistakes[parray->y] && mistakes[parray->y]->offset >= parray->x)
    pop_node(mistakes + parray->y);

  /* 使用空格覆盖当前字符，并将光标防在当前字符 */
  mvwprintw(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x], "  ");
  wmove(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x]);

  wrefresh(win);

  return if_new_line;
}

/* 更新主窗口 */
int update_main_win(WINDOW *win, wchar_t **lines, Node **mistakes,
                    unsigned nline, char **xchar, POS *parray, unsigned ntop,
                    unsigned ndisplay) {

  wclear(win);

  /* draw box and title */
  box(win, 0, 0);
  mvwprintw(win, 0, (main_size.width - wstrwidth(L"[仓颉输入法训练器]")) >> 1,
            "[仓颉输入法训练器]");

  /* draw the referring lines */
  draw_refer_lines(win, lines, nline, xchar, ntop, ndisplay);

  /* draw the completed input lines */
  draw_input_lines(win, lines, nline, mistakes, xchar, parray, ntop, ndisplay);

  wrefresh(win);

  return 1;
}

int main(int argc, char *argv[]) {
  // 设置 locale
  setlocale(LC_ALL, "zh_CN.utf8");

  unsigned num_line;
  wchar_t input;
  wchar_t **lines = read_article("sample.txt", &num_line);
  char **offsets = (char **)malloc(sizeof(char) * num_line);
  for (int i = 0; i < num_line; i++) {
    offsets[i] = (char *)calloc(sizeof(char), wstrlen(lines[i]));
  }
  Node **mistakes = create_table(num_line);

  initscr();
  noecho();
  cbreak();
  refresh();

  WINDOW *popup_win = newwin(popup_size.height, popup_size.width,
                             (WHEIGHT - popup_size.height) / 2,
                             (WWIDTH - popup_size.width) / 2);
  WINDOW *main_win = newwin(main_size.height, main_size.width, 0, 0);
  WINDOW *panel_win =
      newwin(panel_size.height, panel_size.width, 0, main_size.width);

  box(panel_win, 0, 0);
  wrefresh(panel_win);

  POS parray = {4, 2};

  int ntop = 0;

  /* how many lines to display */
  unsigned ndisplay = (main_size.height - 4) / 3;
  update_main_win(main_win, lines, mistakes, num_line, offsets, &parray, ntop,
                  ndisplay);
  WINDOW *hint_win =
      newwin(hint_size.height, hint_size.width, (parray.y - ntop) * 3,
             MAX(offsets[parray.y][parray.x] - 4, 1));
  mvwprintw(hint_win, 1, 1, "%lc %ls", lines[parray.y][parray.x], L"test");
  box(hint_win, 0, 0);
  wrefresh(hint_win);
  delwin(hint_win);
  wmove(main_win, (parray.y - 0) * 3 + 4, offsets[parray.y][parray.x]);
  wrefresh(main_win);

  int if_new_line = 0;

  while (1) {
    wscanf(L"%lc", &input);
    if (input == 127) {
      if_new_line = delete_ch(main_win, lines, num_line, ntop, mistakes,
                              offsets, &parray);
    } else if (input == 'j' && ntop < ndisplay - 1) {
      ntop++;
      update_main_win(main_win, lines, mistakes, num_line, offsets, &parray,
                      ntop, ndisplay);
    } else if (input == 'k' && ntop > 0) {
      ntop--;
      update_main_win(main_win, lines, mistakes, num_line, offsets, &parray,
                      ntop, ndisplay);
    } else {
      if_new_line = input_ch(main_win, lines, num_line, ntop, mistakes, offsets,
                             &parray, input);
    }

    if (if_new_line == 1 && parray.y > ndisplay + ntop - 3 &&
        ntop + ndisplay < num_line) {
      ntop++;
      update_main_win(main_win, lines, mistakes, num_line, offsets, &parray,
                      ntop, ndisplay);
      wrefresh(main_win);
    }

    box(main_win, 0, 0);
    mvwprintw(main_win, 0,
              (main_size.width - wstrwidth(L"[仓颉输入法训练器]")) >> 1,
              "[仓颉输入法训练器]");
    wrefresh(main_win);
    WINDOW *hint_win =
        newwin(hint_size.height, hint_size.width, (parray.y - ntop) * 3,
               MAX(offsets[parray.y][parray.x] - 4, 1));
    mvwprintw(hint_win, 1, 1, "%lc %ls", lines[parray.y][parray.x], L"test");
    box(hint_win, 0, 0);
    wrefresh(hint_win);
    delwin(hint_win);
    wmove(main_win, (parray.y - ntop) * 3 + 4, offsets[parray.y][parray.x]);
    wrefresh(main_win);
  }

  getch();
  endwin();
  destroy_lines(lines, num_line);
  return 0;
}
