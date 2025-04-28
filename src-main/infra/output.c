#include "output.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

// 재귀적으로 디렉토리 생성
int mkdir_recursive(const char *path, mode_t mode)
{
  char temp[1024];

  snprintf(temp, sizeof(temp), "%s", path);
  size_t len = strlen(temp);

  // 마지막에 '/'가 있으면 제거
  if (temp[len - 1] == '/')
    temp[len - 1] = '\0';

  // 하나씩 디렉토리 생성
  char *p = NULL;
  for (p = temp + 1; *p; p++)
  {
    if (*p == '/')
    {
      *p = '\0';
      if (mkdir(temp, mode) != 0)
      {
        if (errno != EEXIST)
        {
          perror("mkdir");
          return -1;
        }
      }
      *p = '/';
    }
  }

  // 마지막 디렉토리 생성
  if (mkdir(temp, mode) != 0)
  {
    if (errno != EEXIST)
    {
      perror("mkdir");
      return -1;
    }
  }

  return 0;
}
