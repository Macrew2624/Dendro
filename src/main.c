#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

struct tree {
  char **names;
  int size;
  int capacity;
};

struct counter {
  int dirs;
  int files;
};

int is_dir(const char *path) {
  struct stat s;
  if (lstat(path, &s) == -1) {
    return 0;
  }
  return S_ISDIR(s.st_mode);
}

int handle_realloc(struct tree *T) {
  if (T->size != T->capacity) {
    return 0;
  }

  int new_capacity = (T->capacity == 0 ? 16 : T->capacity * 2);
  char **temp = realloc(T->names, sizeof(char *) * new_capacity);

  if (temp == NULL) {
    fprintf(stderr, "realloc failed\n");
    return -1;
  }

  T->capacity = new_capacity;
  T->names = temp;
  return 0;
}

int compare(const void *a, const void *b) {
  const char *first = *(const char *const *)a;
  const char *second = *(const char *const *)b;
  return strcmp(first, second);
}

void list_dir(const char *path, const char *prefix, struct counter *C) {
  DIR *stream = opendir(path);
  if (stream == NULL) {
    fprintf(stderr, "Couldn't open dir %s: %s\n", path, strerror(errno));
    return;
  }

  struct dirent *next_dir = readdir(stream);
  char full_path[PATH_MAX] = "";
  char full_prefix[PATH_MAX] = "";

  struct tree T = {NULL, 0, 0};

  while (next_dir != NULL) {
    if (next_dir->d_name[0] == '.') {
      next_dir = readdir(stream);
      continue;
    }

    if (handle_realloc(&T) == -1) {
      for (int i = 0; i < T.size; i++) {
        free(T.names[i]);
      }
      free(T.names);
      closedir(stream);
      return;
    }

    T.names[T.size] = strdup(next_dir->d_name);

    if (T.names[T.size] == NULL) {
      for (int i = 0; i < T.size; i++) {
        free(T.names[i]);
      }
      free(T.names);
      closedir(stream);
      return;
    }

    T.size++;

    next_dir = readdir(stream);
  }

  closedir(stream);

  qsort(T.names, T.size, sizeof(char *), compare);

  for (int i = 0; i < T.size; i++) {
    snprintf(full_path, sizeof(full_path), "%s/%s", path, T.names[i]);
    snprintf(full_prefix, sizeof(full_prefix), "%s%s", prefix,
             i == T.size - 1 ? "    " : "│   ");
    printf("%s", prefix);
    if (i == T.size - 1) {
      printf("└── ");
    } else {
      printf("├── ");
    }
    printf("%s\n", T.names[i]);
    if (is_dir(full_path)) {
      C->dirs++;
      list_dir(full_path, full_prefix, C);
    } else {
      C->files++;
    }
  }

  for (int i = 0; i < T.size; i++) {
    free(T.names[i]);
  }
  free(T.names);
}

int main(int argc, char **argv) {
  char *start = argc > 1 ? argv[1] : ".";
  struct counter C = {1, 0};
  printf("%s\n", start);
  list_dir(start, "", &C);
  printf("\n%d %s, %d %s\n", C.dirs, C.dirs == 1 ? "directory" : "directories",
         C.files, C.files == 1 ? "file" : "files");
  return 0;
}