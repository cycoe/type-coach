#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

int main(int argc, char *argv[])
{
  wchar_t wc; //定义一个宽字符变量，及一个宽字符数组。
  wchar_t ws[1024] = {0};
  char buff[1024] = {0};

  // 设置 locale
  setlocale(LC_ALL, "zh_CN.utf8");

  FILE *fp = fopen("sample.txt", "r");
  fgets(buff, 1024, fp);
  size_t len = mbstowcs(ws, buff, 1024);
  wprintf(L"output is: %ls\n", ws);
  wprintf(L"%ld, %ld\n", strlen(buff), len);

  return 0;
}
