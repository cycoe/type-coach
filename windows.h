#ifndef _WINDOWS_H
#define _WINDOWS_H

#include <ncurses.h>
#include <wchar.h>

int popup_alt(WINDOW *win, const wchar_t *title, const wchar_t *content,
              int default_);
int update_popup_list(WINDOW *win, const wchar_t *title, const wchar_t *content,
                      wchar_t **list, unsigned nitem, unsigned default_,
                      int ntop, int ndisplay);
int popup_list(WINDOW *win, const wchar_t *title, const wchar_t *content,
               wchar_t *list[], unsigned nitem, unsigned default_);

#endif
