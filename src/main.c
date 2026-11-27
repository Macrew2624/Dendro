#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct tree {
  char **names;
  int size;
  int capacity;
};

struct counter {
  int dirs;
  int files;
};

struct args {
  int all, dirs_only, max_depth;
};

void build_path(char *out, size_t size, const char *dir, const char *name) {
  size_t n = strlen(dir);
  snprintf(out, size, "%s%s%s", dir, (n > 0 && dir[n - 1] == '/') ? "" : "/",
           name);
}

int is_dir(const char *path) {
  struct stat s;
  if (lstat(path, &s) == -1) {
    return 0;
  }
  return S_ISDIR(s.st_mode);
}

int points_to_dir(const char *path) {
  struct stat s;
  if (stat(path, &s) == -1) {
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

void list_dir(const char *path, const char *prefix, struct counter *C,
              struct args A) {

  if (A.max_depth == 0) {
    return;
  }

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
    if (strcmp(next_dir->d_name, ".") == 0 ||
        strcmp(next_dir->d_name, "..") == 0 ||
        (A.all == 0 && next_dir->d_name[0] == '.')) {
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

    build_path(full_path, sizeof(full_path), path, next_dir->d_name);
    if (A.dirs_only && !points_to_dir(full_path)) {
      next_dir = readdir(stream);
      continue;
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

  if (T.size > 1) {
    qsort(T.names, T.size, sizeof(char *), compare);
  }

  for (int i = 0; i < T.size; i++) {
    build_path(full_path, sizeof(full_path), path, T.names[i]);

    int dir = is_dir(full_path);
    struct args child = A;
    if (child.max_depth > 0) {
      child.max_depth--;
    }

    char target[PATH_MAX];
    ssize_t n = -1;
    if (!dir) {
      n = readlink(full_path, target, sizeof(target) - 1);
      if (n != -1) {
        target[n] = '\0';
      }
    }

    int unreadable = 0;
    if (dir && child.max_depth != 0) {
      DIR *probe = opendir(full_path);
      if (probe == NULL) {
        unreadable = 1;
      } else {
        closedir(probe);
      }
    }

    snprintf(full_prefix, sizeof(full_prefix), "%s%s", prefix,
             i == T.size - 1 ? "    " : "│   ");
    printf("%s", prefix);
    if (i == T.size - 1) {
      printf("└── ");
    } else {
      printf("├── ");
    }

    printf("%s", T.names[i]);
    if (n != -1) {
      printf(" -> %s", target);
    }
    printf("%s\n", unreadable ? "  [error opening dir]" : "");

    if (dir) {
      C->dirs++;
      if (!unreadable) {
        list_dir(full_path, full_prefix, C, child);
      }
    } else if (n != -1 && points_to_dir(full_path)) {
      C->dirs++;
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
  struct args A = {0, 0, -1};
  int opt;
  while ((opt = getopt(argc, argv, "adL:")) != -1) {
    switch (opt) {
    case 'a':
      A.all = 1;
      break;
    case 'd':
      A.dirs_only = 1;
      break;
    case 'L': {
      char *end;
      long n = strtol(optarg, &end, 10);
      if (*end != '\0' || n < 1 || n > INT_MAX) {
        fprintf(stderr, "invalid level: %s\n", optarg);
        return 1;
      }
      A.max_depth = (int)n;
      break;
    }
    default:
      fprintf(stderr, "usage: %s [-a] [-d] [-L level] [path]\n", argv[0]);
      return 1;
    }
  }
  const char *start = optind < argc ? argv[optind] : ".";
  struct counter C = {1, 0};
  printf("%s\n", start);
  list_dir(start, "", &C, A);
  if (!A.dirs_only) {
    printf("\n%d %s, %d %s\n", C.dirs,
           C.dirs == 1 ? "directory" : "directories", C.files,
           C.files == 1 ? "file" : "files");
  } else {
    printf("\n%d %s\n", C.dirs, C.dirs == 1 ? "directory" : "directories");
  }
  return 0;
}