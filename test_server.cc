#include "tcp.h"

int
main(int argc, char **argv)
{
  tcp::server obj(8081, "127.0.0.1");

  obj.listen();

  char buf[1024];
  int64_t n;
  while (1) {
    int64_t sum(0);
    obj.accept();
    while ((n = obj.read(buf, sizeof(buf))) > 0) {
      sum += n;
      *(buf+n) = 0;
      printf("%s\n", buf);
      n = obj.write(buf, n);
      printf("%ld bytes read.\n", n);
      printf("%ld bytes sent back.\n", n);
    }
    obj.close();
    printf("total %ld bytes read.\n", sum);
  }

  return 0;
}
