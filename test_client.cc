#include "tcp.h"

int
main(int argc, char **argv)
{
  uint N = (argc > 1)?atoi(argv[1]):1;
  tcp::client cli(8081, "127.0.0.1");

  const char* alnum =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  N = (N > strlen(alnum))?strlen(alnum):N;

  char buf[5];
  cli.connect();

  int64_t n,wsum(0),rsum(0);
  for (uint i=1; i<=N; i++) {
    n = cli.write(alnum, i);
    printf("%ld bytes written.\n", n);
    wsum += n;
    n = cli.read(buf, sizeof(buf));
    rsum += n;
    for (int m=0; m<n; m++) printf("%c", buf[m]);
    while ((n = cli.partial_read(buf, sizeof(buf))) > 0) {
      rsum += n;
      for (int m=0; m<n; m++) printf("%c", buf[m]);
    }
    printf("\n");
  }

  cli.close();

  printf("total %lu bytes sent.\n", wsum);
  printf("total %lu bytes read.\n", rsum);
  return 0;
}
