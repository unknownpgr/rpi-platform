#include "output.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

// Recursively create a directory
int mkdir_recursive(const char *path, mode_t mode)
{
  char temp[1024];

  snprintf(temp, sizeof(temp), "%s", path);
  size_t len = strlen(temp);

  // If the last character is '/' remove it
  if (temp[len - 1] == '/')
    temp[len - 1] = '\0';

  // Create a directory one by one
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

  // Create the last directory
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
