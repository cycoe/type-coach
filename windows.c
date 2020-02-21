#include "windows.h"
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

int update_popup_list(WINDOW *win, const wchar_t *title, const wchar_t *content,
                      wchar_t **list, unsigned nitem, unsigned default_,
                      int ntop, int ndisplay) {

  int width = getmaxx(win);
  int height = getmaxy(win);

  wclear(win);

  /* draw borders */
  box(win, 0, 0);
  mvwprintw(win, 0, (width - wstrwidth(title)) >> 1, "%ls", title);

  /* draw title */
  wattron(win, A_BOLD);
  mvwprintw(win, 1, (width - wstrwidth(content)) >> 1, "%ls", content);
  wattroff(win, A_BOLD);

  /* draw list */
  for (int n = ntop; n < ntop + ndisplay && n < nitem; n++) {
    if (n == default_)
      wattron(win, A_REVERSE);
    mvwprintw(win, 3 + n - ntop, 1, "%d. %ls", n + 1, list[n]);
    wattroff(win, A_REVERSE);
  }

  wrefresh(win);

  return 1;
}

int popup_list(WINDOW *win, const wchar_t *title, const wchar_t *content,
              wchar_t *list[], unsigned nitem, unsigned default_) {

  int height = getmaxy(win);
  int width = getmaxx(win);
  int ntop = 0, ndisplay = height - 4;
  int ch;

  keypad(win, TRUE);

  update_popup_list(win, title, content, list, nitem, default_, ntop, ndisplay);

  /* handle input */
  while ((ch = getch())) {
    switch (ch) {
    case KEY_UP:
    case 'k':
      if (default_ > 0) {
        default_--;
        if (default_ < ntop) {
          ntop = default_;
        }
        update_popup_list(win, title, content, list, nitem, default_, ntop,
                          ndisplay);
      }
      break;
    case KEY_DOWN:
    case 'j':
      if (default_ < nitem - 1) {
        default_++;
        if (default_ >= ntop + ndisplay) {
          ntop = default_ - ndisplay + 1;
        }
        update_popup_list(win, title, content, list, nitem, default_, ntop,
                          ndisplay);
      }
      break;
    case 'l':
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
