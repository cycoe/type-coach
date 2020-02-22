#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <malloc.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#include "utils.h"
#include "windows.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct SIZE {
  unsigned height;
  unsigned width;
} SIZE;

typedef struct POS {
  int x;
  int y;
} POS;

typedef struct STAT {
  WINDOW *main_win;
  WINDOW *panel_win;
  time_t time;
  unsigned nchar;
  unsigned nerror;
  unsigned nedit;
} STAT;

#define MALLOC_INIT_SIZE 10
#define MALLOC_INC_STEP 10
#define LINESIZ 4096

#define WWIDTH 80
#define WHEIGHT 40

#define DICT_PATH "dicts/cangjie6.dict.yaml"
#define WORD_PATH "dicts/cangjie6.extsimp.dict.yaml"
#define ARTICLE_DIR "articles/"
#define TITLE_MAIN L"蒼頡檢字法訓練器"
#define TITLE_PANEL L"数据统计"
#define TITLE_KEYBOARD L"键盘提示"

wchar_t base[26] = {L"日月金木水火土的戈十大中一弓人心手口尸廿山女田止卜片"};

SIZE main_size = {26, 60};
SIZE panel_size = {26, 20};
SIZE popup_size = {10, 40};
SIZE keyboard_size = {14, 80};
SIZE button_size = {4, 6};

int wstr_sort_helper(const void *a, const void *b) {

  assert(a != NULL && b != NULL);

  return wstrcmp(*(wchar_t **)a, *(wchar_t **)b);
}

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
  int if_new_line = 0;

  lines = (wchar_t **)calloc(sizeof(wchar_t *), nmalloc);

  FILE *fp = fopen(article_file, "r");
  if (fp == NULL)
    return NULL;

  nstore = 0;
  nlen = 0;
  line = (wchar_t *)calloc(sizeof(wchar_t), main_size.width - 3);
  while (fgets(sbuf, LINESIZ, fp)) {
    /* skip empty lines */
    if (*sbuf == '\n')
      continue;

    nw = mbstowcs(wbuf, sbuf, LINESIZ);
    noffset = 0;
    while (noffset < nw) {
      /* skip returns */
      if (wbuf[noffset] == '\n') {
        if_new_line = 1;
        noffset++;
      } else {
        line[nstore] = wbuf[noffset++];
        if (line[nstore] < 256) {
          nlen += 1;
        } else {
          nlen += 2;
        }
        nstore++;
      }

      if (if_new_line || nlen == main_size.width - 4 ||
          (nlen == main_size.width - 5 && wbuf[noffset] > 256)) {
        if_new_line = 0;
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

  fclose(fp);
  *num_line = nline;
  return lines;
}

char **load_codes(const char *dict_file, wchar_t *startch, unsigned *ncode) {
  char **codes, *code;
  wchar_t ch, maxch, minch;
  wchar_t wbuf[LINESIZ];
  char sbuf[LINESIZ];
  fpos_t fstartpos;

  FILE *fp = fopen(dict_file, "r");
  if (fp == NULL)
    return NULL;

  fgetpos(fp, &fstartpos);

  /* 第一次扫描读取码表中编码最大和最小的字符 */
  maxch = 0, minch = -1;
  while (fgetws(wbuf, LINESIZ, fp)) {
    ch = wbuf[0];
    maxch = MAX(maxch, ch);
    minch = MIN(minch, ch);
  }
  *ncode = maxch - minch + 1;
  *startch = minch;
  fprintf(stderr, "ch: %u, maxch: %u, minch: %u, ncode: %u\n", ch, maxch, minch,
          *ncode);

  /* 为所有的编码分配一个数组 */
  codes = (char **)malloc(sizeof(char *) * *ncode);

  fsetpos(fp, &fstartpos);
  while (fgetws(wbuf, LINESIZ, fp)) {
    wcstombs(sbuf, wbuf + 1, LINESIZ);
    code = (char *)calloc(sizeof(char), 7);

    /* 对于仓颉输入法，`'` 用于表示末尾的包围结构，
     * 因此可能存在两个编码，此处需要找到最后一个编码*/
    int offset = strlen(sbuf) - 1;
    while (sbuf[offset] > 'z' || sbuf[offset] < 'a') {
      offset--;
    }
    /* 找到最后一个编码的开头 */
    while ((sbuf[offset] >= 'a' && sbuf[offset] <= 'z') ||
           sbuf[offset] == '\'') {
      offset--;
    }
    sscanf(sbuf + offset, "%s", code);
    codes[wbuf[0] - minch] = code;
  }

  return codes;
}

wchar_t **load_words(const char *word_file, unsigned *nword) {

  wchar_t **words, *word;
  wchar_t wbuf[LINESIZ] = {0};
  fpos_t fstartpos;

  FILE *fp = fopen(word_file, "r");
  if (fp == NULL)
    return NULL;

  fgetpos(fp, &fstartpos);

  *nword = 0;
  while (fgetws(wbuf, LINESIZ, fp)) {
    (*nword)++;
  }

  words = (wchar_t **)malloc(sizeof(wchar_t *) * *nword);

  fsetpos(fp, &fstartpos);
  for (int j = 0; fgetws(wbuf, LINESIZ, fp); j++) {
    word = (wchar_t *)calloc(sizeof(wchar_t), 6);
    for (int i = 0; wbuf[i] > 255; i++) {
      word[i] = wbuf[i];
    }
    words[j] = word;
  }

  return words;
}

char *get_code(char **codes, wchar_t startch, unsigned ncode, wchar_t ch) {

  if (ch < startch || ch >= startch + ncode) {
    return NULL;
  }

  return codes[ch - startch];
}

wchar_t *search_word(wchar_t *word_to_find, wchar_t **words, unsigned nword) {
  int left, right, mid;

  left = 0, right = nword - 1;
  while (left <= right) {
    mid = (left + right) / 2;
    if (wstrcmp(word_to_find, words[mid]) < 0)
      right = mid - 1;
    else if (wstrcmp(word_to_find, words[mid]) > 0)
      left = mid + 1;
    else
      return words[mid];
  }

  return NULL;
}

char *get_head(const char *code, char *rcode) {
  memset(rcode, 0, 2);
  rcode[0] = code[0];

  return rcode;
}

char *get_tail(const char *code, char *rcode) {
  char *quot;
  unsigned code_len;

  code_len = strlen(code);

  memset(rcode, 0, 2);
  if ((quot = strchr(code, '\''))) {
    rcode[0] = *(quot - 1);
  } else {
    rcode[0] = code[code_len - 1];
  }

  return rcode;
}

char *get_head_tail(const char *code, char *rcode) {
  char *quot;
  unsigned code_len;

  memset(rcode, 0, 3);

  code_len = strlen(code);
  rcode[0] = code[0];

  if ((quot = strchr(code, '\''))) {
    if (quot - code > 1)
      rcode[1] = *(quot - 1);
    else
      rcode[1] = code[code_len - 1];
  } else if (code_len > 1) {
    rcode[1] = code[code_len - 1];
  }

  return rcode;
}

char *get_head_body_tail(const char *code, char *rcode) {
  char *quot;
  unsigned code_len;

  memset(rcode, 0, 4);

  code_len = strlen(code);
  rcode[0] = code[0];

  if ((quot = strchr(code, '\''))) {
    if (quot - code == 1) {
      rcode[1] = code[code_len - 1];
    } else if (quot - code == 2) {
      rcode[1] = code[1];
      rcode[2] = code[code_len - 1];
    } else {
      rcode[1] = code[1];
      rcode[2] = *(quot - 1);
    }
  } else if (code_len == 2) {
    rcode[1] = code[1];
  } else if (code_len > 2) {
    rcode[1] = code[1];
    rcode[2] = code[code_len - 1];
  }

  return rcode;
}

int get_word_code(const wchar_t *word, char *word_code, char **codes, wchar_t startch,
              unsigned ncode) {
  char *code = NULL, *head_tail;
  unsigned word_len;

  word_len = wstrlen(word);

  head_tail = (char *)calloc(sizeof(char), 4);

  if (word_len == 2) {
    code = get_code(codes, startch, ncode, word[0]);
    get_head_tail(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[1]);
    get_head_body_tail(code, head_tail);
    strcat(word_code, head_tail);
  } else if (word_len == 3) {
    code = get_code(codes, startch, ncode, word[0]);
    get_head_tail(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[1]);
    get_head_tail(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[2]);
    get_tail(code, head_tail);
    strcat(word_code, head_tail);
  } else if (word_len == 4) {
    code = get_code(codes, startch, ncode, word[0]);
    get_head(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[1]);
    get_tail(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[2]);
    get_head_tail(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[3]);
    get_tail(code, head_tail);
    strcat(word_code, head_tail);
  } else {
    code = get_code(codes, startch, ncode, word[0]);
    get_head(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[1]);
    get_tail(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[2]);
    get_head(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[3]);
    get_tail(code, head_tail);
    strcat(word_code, head_tail);
    code = get_code(codes, startch, ncode, word[word_len - 1]);
    get_tail(code, head_tail);
    strcat(word_code, head_tail);
  }

  return 0;
}

char **get_words_code(wchar_t **words, unsigned nword, char **words_code,
                      char **codes, wchar_t startch, unsigned ncode) {

  char *code;
  int i;

  for (i = 0; i < nword; i++) {
    code = (char *)calloc(sizeof(char), 6);
    get_word_code(words[i], code, codes, startch, ncode);
    words_code[i] = code;
  }

  return words_code;
}

int get_word_after_cursor(wchar_t **lines, unsigned nline, POS *parray,
                          char **codes, wchar_t startch, unsigned ncode,
                          wchar_t *word) {
  unsigned word_len;
  int y, x;

  y = parray->y;
  x = parray->x;

  word_len = 0;
  while ((y < nline || lines[y][x]) &&
         get_code(codes, startch, ncode, lines[y][x]) && word_len < 5) {
    word[word_len++] = lines[y][x++];
    while (lines[y][x] == 0) {
      y++;
      x = 0;
    }
  }

  return word_len;
}

/* free the memory of lines */
void destroy_lines(wchar_t **lines, unsigned nline) {
  for (int i = 0; i < nline; i++)
    free(lines[i]);
  free(lines);
}

void destroy_codes(char **codes, unsigned ncode) {
  for (int i = 0; i < ncode; i++)
    free(codes[i]);
  free(codes);
}

void convert_code(wchar_t *converted_code, char *code) {
  int i, j;
  char ch;

  memset(converted_code, 0, 6);
  i = 0, j = 0;

  while ((ch = code[i++])) {
    if (ch - 'a' >= 0 && ch - 'a' < 26) {
      converted_code[j++] = base[ch - 'a'];
    }
  }
}

char **create_xchar(wchar_t * *lines, unsigned nline) {
    char **xchar;

    xchar = (char **)malloc(sizeof(char *) * nline);
    for (int i = 0; i < nline; i++) {
      xchar[i] = (char *)calloc(sizeof(char), wstrlen(lines[i]));
    }

    return xchar;
  }

  /* draw the referring lines */
  int draw_refer_lines(WINDOW * win, wchar_t * *lines, unsigned nline,
                       char **xchar, unsigned ntop, unsigned ndisplay) {
    wchar_t *line;

    /* 绘制屏幕上显示的所有参考行 */
    for (unsigned y = ntop; y < ntop + ndisplay; y++) {
      /* 开启下划线 */
      wattron(win, A_UNDERLINE);

      line = lines[y];
      wmove(win, 3 * (y - ntop) + 3, 2);
      for (unsigned x = 0; line[x]; x++) {
        /* 记录每个字符在屏幕上的位置 */
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

  init_pair(1, COLOR_BLACK, COLOR_RED);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);

  /* 储存当前光标所在位置字符，并将当前光标所在位置置为 0 */
  temp = lines[parray->y][parray->x];
  lines[parray->y][parray->x] = 0;

  for (unsigned y = ntop; y <= parray->y && y < ntop + ndisplay; y++) {
    /* 打印屏幕上显示的第一行到当前行的原始文本 */
    if (has_colors())
      wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 3 * (y - ntop) + 4, 2, "%ls", lines[y]);
    wattroff(win, COLOR_PAIR(2));

    /* 分三种情况显示错误文本 */
    if (has_colors())
      wattron(win, COLOR_PAIR(1));
    else
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
    wattroff(win, COLOR_PAIR(1));
    wattroff(win, A_REVERSE);
  }

  /* 恢复当前光标的字符 */
  lines[parray->y][parray->x] = temp;

  /* 设置光标位置 */
  wmove(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x]);

  return 1;
}

int draw_border(WINDOW *win, const SIZE *win_size, const wchar_t *title) {
  unsigned title_width;

  box(win, 0, 0);

  if (title) {
    title_width = wstrwidth(title) + 2;
    mvwprintw(win, 0, (win_size->width - title_width) / 2, "[%ls]", title);
  }

  wrefresh(win);
  return 1;
}

int draw_process(WINDOW *win, SIZE *win_size, float process) {

  unsigned index = (win_size->width - 2) * process;

  wmove(win, win_size->height - 2, 1);
  for (int i = 0; i < index; i++) {
    wprintw(win, "▉");
  }
  for (int i = index; i < win_size->width - 2; i++) {
    wprintw(win, " ");
  }

  wrefresh(win);
  return 1;
}

int input_ch(WINDOW *win, wchar_t **lines, unsigned nline, unsigned ntop,
             Node **mistakes, char **xchar, POS *parray, STAT *stat,
             wchar_t input) {

  int if_new_line = 0;

  init_pair(1, COLOR_BLACK, COLOR_RED);
  init_pair(2, COLOR_BLACK, COLOR_GREEN);

  /* 如果输入字符与参考字符不匹配 */
  if (input != lines[parray->y][parray->x]) {
    /* 将该字符压入错误栈 */
    push_table(mistakes + parray->y, parray->x, input);
    /* 开启反色模式 */
    wattron(win, COLOR_PAIR(1));
    stat->nerror++;
  } else {
    wattron(win, COLOR_PAIR(2));
  }
  stat->nchar++;

  /* 分三种情况打印字符 */
  wmove(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x]);
  if (input > 255 && lines[parray->y][parray->x] < 256) {
    wprintw(win, "x");
  } else if (input < 256 && lines[parray->y][parray->x] > 255) {
    wprintw(win, "%lc ", input);
  } else {
    wprintw(win, "%lc", input);
  }
  wattroff(win, COLOR_PAIR(1));
  wattroff(win, COLOR_PAIR(2));
  use_default_colors();
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
              Node **mistakes, char **xchar, POS *parray, STAT *stat) {

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
  while (mistakes[parray->y] && mistakes[parray->y]->offset >= parray->x) {
    pop_node(mistakes + parray->y);
    stat->nedit++;
    stat->nerror--;
  }
  stat->nchar--;

  /* 使用空格覆盖当前字符，并将光标防在当前字符 */
  mvwprintw(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x], "  ");
  wmove(win, (parray->y - ntop) * 3 + 4, xchar[parray->y][parray->x]);

  wrefresh(win);

  return if_new_line;
}

unsigned count_ch(wchar_t **lines, unsigned nline) {

  unsigned count = 0;

  for (int i = 0; i < nline; i++) {
    count += wstrlen(lines[i]);
  }

  return count;
}

/* 更新主窗口 */
int update_main_win(WINDOW *win, wchar_t **lines, Node **mistakes,
                    unsigned nline, char **xchar, POS *parray, unsigned ntop,
                    unsigned ndisplay) {

  wclear(win);

  /* draw box and title */
  draw_border(win, &main_size, L"仓颉输入法训练器");

  /* draw the referring lines */
  draw_refer_lines(win, lines, nline, xchar, ntop, ndisplay);

  /* draw the completed input lines */
  draw_input_lines(win, lines, nline, mistakes, xchar, parray, ntop, ndisplay);

  draw_process(win, &main_size,
               (float)(count_ch(lines, parray->y) + parray->x) /
               count_ch(lines, nline));

  wrefresh(win);

  return 1;
}

int update_panel_win(WINDOW *win, STAT *stat) {
  mvwprintw(win, 3, 2, "已输入字数  %u", stat->nchar);
  mvwprintw(win, 4, 2, "错误字数    %u", stat->nerror);
  mvwprintw(win, 5, 2, "回改字数    %u", stat->nedit);

  draw_border(win, &panel_size, L"数据统计");

  wrefresh(win);

  return 1;
}

int update_keyboard_win(WINDOW *win, char *code) {

  static char alphabets[26] = "qwertyuiopasdfghjklzxcvbnm";
  WINDOW *button_win;
  int y, x;

  draw_border(win, &keyboard_size, L"键盘提示");

  for (int i = 0; i < 26; i++) {
    if (i < 10) {
      y = main_size.height + 1;
      x = (keyboard_size.width - 60) / 2 + button_size.width * i;
    } else if (i < 19) {
      y = main_size.height + 5;
      x = (keyboard_size.width - 56) / 2 + button_size.width * (i - 10);
    } else {
      y = main_size.height + 9;
      x = (keyboard_size.width - 50) / 2 + button_size.width * (i - 19);
    }

    button_win = newwin(button_size.height, button_size.width, y, x);
    if (code && strchr(code, alphabets[i])) {
      wattron(button_win, A_REVERSE);
    }
    mvwprintw(button_win, 1, 1, "  %c ", toupper(alphabets[i]));
    mvwprintw(button_win, 2, 1, " %lc ", base[alphabets[i] - 'a']);
    box(button_win, 0, 0);
    wattroff(button_win, A_REVERSE);
    wrefresh(button_win);
    delwin(button_win);
  }

  return 1;
}

wchar_t **check_word_in_words(wchar_t *long_word, unsigned word_len,
                            wchar_t **words, unsigned nword,
                            wchar_t **word_in_words,
                            unsigned *nword_in_words_) {

  /* record the how many words are in the word list */
  unsigned nword_in_words = 0;

  /* 扫描最长词组中可能的词组 */
  while (word_len > 1) {
    /* 如果当期的词在词组列表中，将其加入到可用词组列表 */
    if (search_word(long_word, words, nword)) {
      word_in_words[nword_in_words] = (wchar_t *)calloc(sizeof(wchar_t), 6);
      for (int i = 0; long_word[i]; i++) {
        word_in_words[nword_in_words][i] = long_word[i];
      }
      nword_in_words++;
    }
    /* 清除掉最后一个字 */
    long_word[--word_len] = 0;
  }

  *nword_in_words_ = nword_in_words;
  return word_in_words;
}

int update_hint_win(WINDOW *main_win, wchar_t ch, char *code, wchar_t **words,
                    unsigned nword, char **words_code, unsigned pscreeny,
                    unsigned pscreenx) {

  unsigned max_word_width, max_code_width;

  /* if there is no code for current character, don't popup the hint */
  if (code == NULL) {
    /* 恢复光标位置 */
    wmove(main_win, pscreeny, pscreenx);
    wrefresh(main_win);
    return 0;
  }

  /* 转换编码 */
  wchar_t converted_code[6] = {0};
  convert_code(converted_code, code);

  max_word_width = 2;
  max_code_width = strlen(code);

  for (int i = 0; i < nword; i++) {
    max_word_width = MAX(max_word_width, wstrwidth(words[i]));
    max_code_width = MAX(max_code_width, strlen(words_code[i]));
  }

  /* 计算宽度 */
  SIZE hint_size;
  hint_size.height = 3 + nword;
  hint_size.width = 4 + max_word_width + max_code_width * 3;

  /* 确定提示窗口位置 */
  int hint_y = pscreeny > main_size.height / 2 ? pscreeny - hint_size.height - 1
                                               : pscreeny + 1;
  int hint_x = pscreenx - (hint_size.width - 1) / 2;
  /* 检测与边界的重叠 */
  if (hint_x < 1)
    hint_x = 1;
  else if (hint_x > main_size.width - hint_size.width - 2)
    hint_x = main_size.width - hint_size.width - 2;

  /* 绘制提示窗口 */
  WINDOW *hint_win = newwin(hint_size.height, hint_size.width, hint_y, hint_x);
  mvwprintw(hint_win, 1 + nword, 1, "%lc", ch);
  mvwprintw(hint_win, 1 + nword, 2 + max_word_width, "%s", code);
  mvwprintw(hint_win, 1 + nword, 3 + max_word_width + max_code_width, "%ls",
            converted_code);
  while (nword-- > 0) {
    convert_code(converted_code, words_code[nword]);
    mvwprintw(hint_win, 1 + nword, 1, "%ls", words[nword]);
    mvwprintw(hint_win, 1 + nword, 2 + max_word_width, "%s", words_code[nword]);
    mvwprintw(hint_win, 1 + nword, 3 + max_word_width + max_code_width, "%ls",
              converted_code);
  }
            box(hint_win, 0, 0);
  wrefresh(hint_win);
  delwin(hint_win);

  /* 恢复光标位置 */
  wmove(main_win, pscreeny, pscreenx);
  wrefresh(main_win);

  return 0;
}

wchar_t **choose_article(WINDOW *win, unsigned *nline) {
  wchar_t **lines; // lines of article content
  wchar_t **files; // array to store file name of article files
  unsigned nfile;  // amount of article files
  int select;      // your selection of articles

  /* load files from articles directory */
  files = listdir("articles/", &nfile);
  /* if there is no article, return NULL */
  if (nfile == 0) {
    return NULL;
  }

  /* select an article */
  select = popup_list(win, L"菜单", L"选择一篇训练文章", files, nfile, 0);

  /* generate the full path of article file */
  char fullpath[FILENAME_MAX] = ARTICLE_DIR;
  char filename[FILENAME_MAX] = {0};
  wcstombs(filename, files[select], FILENAME_MAX);
  strcat(fullpath, filename);

  /* load lines */
  lines = read_article(fullpath, nline);

  return lines;
}

/* DONE 提示窗口的位置
 * DONE 文章选单
 * TODO 码表选单（暂缓）
 * TODO 上下控制滾屏（暂缓）
 * DONE 删除文字时向上滚屏
 * DONE 打字进度条
 * DONE 完善彩色模式
 * TODO 键位练习模式
 * TODO 可关闭提示
 * DONE 词组提示（实现二分查找）
 * DONE 提示窗口自适应性位置
 * TODO 数据统计
 * DONE 自动跳过空行
 * TODO 仓颉单字编码和词组编码不同
 */

int init_ui(WINDOW *main_win, SIZE *main_size, WINDOW *panel_win, SIZE *panel_size,
         WINDOW *keyboard_win, SIZE *keyboar_size) {

  draw_border(main_win, main_size, TITLE_MAIN);

  draw_border(panel_win, panel_size, TITLE_PANEL);

  update_keyboard_win(keyboard_win, NULL);

  return 1;
}

void *stat_by_time(void *arg) {
  STAT *stat = (STAT *)arg;
  time_t now;
  unsigned cursory, cursorx;
  int time_spend;

  while (1) {
    cursory = getcury(stat->main_win);
    cursorx = getcurx(stat->main_win);

    now = time(NULL);
    time_spend = (int)difftime(now, stat->time);
    mvwprintw(stat->panel_win, 2, 2, "%ls      %d", L"总用时", time_spend);
    mvwprintw(stat->panel_win, 6, 2, "%ls    %d", L"输入速度",
              stat->nchar * 60 / (time_spend + 1));

    wmove(stat->main_win, cursory, cursorx);
    wrefresh(stat->panel_win);
    wrefresh(stat->main_win);

    sleep(1);
  }

return NULL;
}

int main(int argc, char *argv[]) {

  // set locale so that ncurses can print chinese characters
  setlocale(LC_ALL, "zh_CN.utf8");

  /* lines from article */
  static wchar_t **lines;
  /* amount of lines */
  static unsigned nline;

  /* codes from IM code table */
  static char **codes;
  /* amount of codes */
  static unsigned ncode;
  /* codes start from startch */
  static wchar_t startch;
  /* character input from keyboard */
  static wchar_t input;

  static wchar_t **words;
  static unsigned nword;

  /* struct to store input analysis */
  static STAT stat;
  /* struct to store the current cursor in lines array */
  static POS parray = {0, 0};
  /* store the postion on the screen of each character */
  static char **xchar;
  /* store the mistake characters */
  static Node **mistakes;

  /* the index of line displayed on the top of window */
  static int ntop = 0;
  /* how many lines to display */
  static unsigned ndisplay;
  ndisplay = (main_size.height - 4) / 3;

  /* windows */
  static WINDOW *main_win, *panel_win, *keyboard_win, *popup_win;

  /* TODO 整理 */
  wchar_t word_after_cursor[6];
  wchar_t *words_display[5];
  unsigned nwords_display;
  char *words_code[5];

  /* init ncurses window */
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, TRUE);
  /* if has colors, use them */
  if (has_colors()) {
    start_color();
    use_default_colors();
  }

  /* you must refresh the stdscr before all other windows,
   * or all of them will not display */
  refresh();

  /* generate all windows */
  main_win = newwin(main_size.height, main_size.width, 0, 0);
  panel_win = newwin(panel_size.height, panel_size.width, 0, main_size.width);
  keyboard_win =
      newwin(keyboard_size.height, keyboard_size.width, main_size.height, 0);
  popup_win = newwin(popup_size.height, popup_size.width,
                     (WHEIGHT - popup_size.height) / 2,
                     (WWIDTH - popup_size.width) / 2);

  /* load codes */
  codes = load_codes(DICT_PATH, &startch, &ncode);
  words = load_words(WORD_PATH, &nword);
  qsort(words, nword, sizeof(wchar_t *), wstr_sort_helper);

  /* init the ui, as a background of the program */
  init_ui(main_win, &main_size, panel_win, &panel_size, keyboard_win,
          &keyboard_size);

  stat.main_win = main_win, stat.panel_win = panel_win, stat.nchar = 0,
  stat.nerror = 0, stat.nedit = 0;
  lines = choose_article(popup_win, &nline);
  xchar = create_xchar(lines, nline);
  mistakes = create_table(nline);

  unsigned word_len;
  word_len = get_word_after_cursor(lines, nline, &parray, codes, startch, ncode,
                                   word_after_cursor);
  check_word_in_words(word_after_cursor, word_len, words, nword, words_display,
                      &nwords_display);
  get_words_code(words_display, nwords_display, words_code, codes, startch,
                 ncode);

  update_main_win(main_win, lines, mistakes, nline, xchar, &parray, ntop,
                  ndisplay);
  update_panel_win(panel_win, &stat);
  update_keyboard_win(keyboard_win, codes[lines[parray.y][parray.x] - startch]);
  update_hint_win(main_win, lines[parray.y][parray.x],
                  get_code(codes, startch, ncode, lines[parray.y][parray.x]),
                  words_display, nwords_display, words_code,
                  (parray.y - ntop) * 3 + 4, xchar[parray.y][parray.x]);

  /* 开始计时 */
  pid_t pid;
  pthread_t ptd;
  stat.time = time(NULL);
  pid = pthread_create(&ptd, NULL, stat_by_time, &stat);

  int if_new_line = 0;

  while (1) {
    if_new_line = 0;
    input = getwchar();

    /* escape */
    if (input == 27) {
      destroy_lines(lines, nline);
      destroy_table(mistakes, nline);

      parray.x = 0, parray.y = 0;
      lines = choose_article(popup_win, &nline);
      mistakes = create_table(nline);

      update_main_win(main_win, lines, mistakes, nline, xchar, &parray, ntop,
                      ndisplay);
      update_panel_win(panel_win, &stat);
      update_keyboard_win(keyboard_win,
                          codes[lines[parray.y][parray.x] - startch]);
      update_hint_win(
          main_win, lines[parray.y][parray.x],
          get_code(codes, startch, ncode, lines[parray.y][parray.x]),
          words_display, nwords_display, words_code, (parray.y - ntop) * 3 + 4,
          xchar[parray.y][parray.x]);
    } else if (input == 127) {
      /* 退格键 */
      if_new_line = delete_ch(main_win, lines, nline, ntop, mistakes, xchar,
                              &parray, &stat);
    } else {
      if_new_line = input_ch(main_win, lines, nline, ntop, mistakes, xchar,
                             &parray, &stat, input);
    }

    if (if_new_line == 1 && parray.y > ndisplay + ntop - 3 &&
        ntop + ndisplay < nline) {
      ntop = parray.y - ndisplay + 3;
      update_main_win(main_win, lines, mistakes, nline, xchar, &parray, ntop,
                      ndisplay);
    } else if (if_new_line == 1 && parray.y < ntop + 2 && ntop > 0) {
      ntop = MAX(parray.y - 2, 0);
      update_main_win(main_win, lines, mistakes, nline, xchar, &parray, ntop,
                      ndisplay);
    }

    draw_border(main_win, &main_size, TITLE_MAIN);
    draw_process(main_win, &main_size,
                 (float)(count_ch(lines, parray.y) + parray.x) /
                     count_ch(lines, nline));

    update_panel_win(panel_win, &stat);
    update_keyboard_win(keyboard_win, get_code(codes, startch, ncode,
                                               lines[parray.y][parray.x]));
    word_len = get_word_after_cursor(lines, nline, &parray, codes, startch,
                                     ncode, word_after_cursor);
    check_word_in_words(word_after_cursor, word_len, words, nword,
                        words_display, &nwords_display);
    get_words_code(words_display, nwords_display, words_code, codes, startch,
                   ncode);
    update_hint_win(main_win, lines[parray.y][parray.x],
                    get_code(codes, startch, ncode, lines[parray.y][parray.x]),
                    words_display, nwords_display, words_code,
                    (parray.y - ntop) * 3 + 4, xchar[parray.y][parray.x]);
  }

  getch();
  endwin();
  destroy_lines(lines, nline);
  destroy_codes(codes, ncode);
  return 1;
}
