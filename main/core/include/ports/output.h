#pragma once

int output_create(char *filename);
void output_write(int fd, char *data);
void output_close(int fd);
