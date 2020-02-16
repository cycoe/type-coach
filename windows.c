#include <ncurses.h>
#include <wchar.h>

#include "utils.h"

int popup_alt(WINDOW *win, const wchar_t *title, const wchar_t *content,
              int default_) {

  int width = getmaxx(win);
  int height = getmaxy(win);

  wrefresh(win);

  /* draw title */
  wattron(win, A_BOLD);
  mvwprintw(win, 1, (width - wstrwidth(title)) >> 1, "%ls", title);
  wattroff(win, A_BOLD);

  /* draw content */
  mvwprintw(win, 3, 1, "%ls", content);

  /* draw buttons */
  wattron(win, A_UNDERLINE);
  mvwprintw(win, height - 2, width - 4, default_ ? "Y/n" : "y/N");
  wattroff(win, A_UNDERLINE);

  /* draw borders */
  box(win, 0, 0);

  /* refresh window */
  wrefresh(win);

  /* handle input */
  int ch = getch();
  switch (ch) {
  case 'y':
    return 1;
    break;
  case 'n':
    return 0;
    break;
  case KEY_ENTER:
  default:
    return default_;
    break;
  }
}

int popup_list(WINDOW *win, const wchar_t *title, const wchar_t *list[],
               unsigned nitem, unsigned default_) {

  int width = getmaxx(win);
  int height = getmaxy(win);

  /* draw title */
  wattron(win, A_BOLD);
  mvwprintw(win, 1, (width - wstrwidth(title)) >> 1, "%ls", title);
  wattroff(win, A_BOLD);

  /* draw list */
  for (int n = 0; n < nitem; n++) {
    if (n == default_)
      wattron(win, A_REVERSE);
    mvwprintw(win, 3 + n, 1, "%d. %ls", n + 1, list[n]);
    wattroff(win, A_REVERSE);
  }

  /* draw borders */
  box(win, 0, 0);

  /* refresh window */
  wrefresh(win);

  keypad(stdscr, 1);
  /* handle input */
  int ch;
  while ((ch = getch())) {
    switch (ch) {
    case KEY_UP:
    case 'k':
      if (default_ > 0) {
        mvwprintw(win, 3 + default_, 1, "%d. %ls", default_ + 1,
                  list[default_]);
        default_--;
        wattron(win, A_REVERSE);
        mvwprintw(win, 3 + default_, 1, "%d. %ls", default_ + 1,
                  list[default_]);
        wattroff(win, A_REVERSE);
        wrefresh(win);
      }
      break;
    case KEY_DOWN:
    case 'j':
      if (default_ < nitem - 1) {
        mvwprintw(win, 3 + default_, 1, "%d. %ls", default_ + 1,
                  list[default_]);
        default_++;
        wattron(win, A_REVERSE);
        mvwprintw(win, 3 + default_, 1, "%d. %ls", default_ + 1,
                  list[default_]);
        wattroff(win, A_REVERSE);
        wrefresh(win);
      }
      break;
    case KEY_ENTER:
    case KEY_RIGHT:
      return default_;
      break;
    default:
      break;
    }
  }

  return default_;
}
