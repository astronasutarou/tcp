/**
 * @file tcp.h
 * @brief TCP を利用してデータ通信をおこなうためのクラスを提供する
 * @author Ryou OHSAWA
 * @date 2015
 *
 * TCP プロトコルを使用してデータ通信をおこなうためのクラスを提供する．
 *
 * このヘッダでは以下のクラスを提供する:
 * - tcp_receiver: TCP プロトコルでデータを受信する
 * - tcp_transmitter: TCP プロトコルでデータを送信する
 * - udp_receiver: UDP プロトコルでデータを受信する
 * - udp_transmitter: UDP プロトコルでデータを送信する
 *
 * コンストラクタでは基本的に IP アドレスを string で port を int で指定する．
 * レシーバは read(buf, N) という関数でデータを受信する．ここで buf は読みだした
 * データを保存するための領域であり N は読みだすデータの最大容量である．
 * トランスミッタは write(data, N) という関数でデータを送信する．ここで data は
 * 送信するためのデータを保持するバッファであり N は送信するデータ数である．
 */
#ifndef H_CONNECTION_TCP
#define H_CONNECTION_TCP

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/**
 * @brief ネットワーク越しにデータ通信するためのクラス
 */
namespace tcp
{
  /** @brief 書き込みに使用するデフォルトのバッファサイズ */
  constexpr size_t BUFSIZE = 2880;

  /**
   * @brief TCP 通信の基本的な機能を実装した基底クラス
   * @note このクラスは基本的な関数だけを規定する virtual なクラス
   */
  class connection
  {
  public:
    /**
     * @brief ポート番号 @c port_ との通信を開始する
     * @param port_ ポート番号
     */
    connection(const int port_)
      : port(port_), status(INITIALIZED) {}
    ~connection() {}

    /** @brief 現在のポート番号を取得する */
    const int
    get_port() const
    { return port; }

    /**
     * @brief 現在の通信ソケットのディスクリプタを取得する
     */
    const int
    get_socket() const
    { return sockfd; }

    /**
     * @brief @c port から @c buf に @c n bytes のデータを読み込む
     * @param buf データを保存するためのバッファ
     * @param n 読み込むデータの容量 (byte)
     * @return 実際に読み込んだデータの容量 (byte)
     */
    const int
    read(void* buf, const int n)
    {
      if ((status & CONNECTED) == 0) {
        fprintf(stderr, "error: not connected yet.\n");
        exit(1);
      }
      int &&num = ::read(sockfd, buf, n);
      return num;
    };
    template <class T>
    const int
    read(T* buf, const int n)
    { return read((void*)buf, n); }

    template <class T>
    const int
    partial_read(T* buf, const int n)
    {
      set_nonblock();
      int &&num = read((void*)buf,n);
      unset_nonblock();
      return num;
    }

    /**
     * @brief @c port に @c buf の先頭から @c n bytes のデータを書き込む
     * @param buf 書き込むためのデータ保持しているバッファ
     * @param n 書き込むデータの容量 (byte)
     * @return 実際に書き込んだデータの容量 (byte)
     */
    const int
    write(const void* data, const int n)
    {
      if ((status & CONNECTED) == 0) {
        fprintf(stderr, "error: not connected yet.\n");
        exit(1);
      }
      int &&num = ::write(sockfd, data, n);
      return num;
    }
    template <class T>
    const int
    write(const T* data, const int n)
    { return write((const void*)data, n); }

    void
    set_nonblock()
    {
      int flag = fcntl(sockfd, F_GETFL, 0);
      fcntl(sockfd, F_SETFL, flag |= O_NONBLOCK);
    }

    void
    unset_nonblock()
    {
      int flag = fcntl(sockfd, F_GETFL, 0);
      fcntl(sockfd, F_SETFL, flag &= ~O_NONBLOCK);
    }

  protected:
    /** @brief 通信を担当するソケットのディスクリプタ */
    int sockfd;
    /** @brief ポート番号 */
    int port;
    /** @brief クラスの状態を保存する変数 */
    uint status;
    /** @brief クラスの状態を表現する列挙型 */
    enum {
      INITIALIZED = 0x00, /**< @brief 初期化された */
      INIT_SOCKET = 0x01, /**< @brief sockfd が初期化された */
      INIT_SERVER = 0x02, /**< @brief serverfd が初期化された */
      LISTENING   = 0x04, /**< @brief listen 状態 (server) */
      CONNECTED   = 0x08, /**< @brief 接続に成功した */
    };
  private:
  };


  /**
   * @brief サーバとして機能するクラス提供する
   */
  class server
    : public tcp::connection
  {
  public:
    /**
     * @brief TCP プロトコルでサーバとして機能する
     * @param port_ 通信するポート番号
     * @param ipaddr_ 接続を許可するクライアントの IP アドレス
     * @note ipaddr_ が NULL の場合には IP アドレスに制限を加えない
     */
    server(const int port_, const char* ipaddr_ = NULL)
      : tcp::connection(port_)
    {
      init_connection(port_, ipaddr_);
    }

    ~server()
    {
      close_socket();
    }

    void
    init_connection(const int port_, const char* ipaddr_ = NULL)
    {
      init_socket();
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(port);
      if (ipaddr_ == NULL) {
        server_addr.sin_addr.s_addr = INADDR_ANY;
      } else {
        server_addr.sin_addr.s_addr = inet_addr(ipaddr_);
      }

      /*
       * 一度接続に成功すると一定時間 port が専有される．
       * SO_REUSEADDR というオプションを有効にすることで port が使われていても
       * 上書きして port に bind することができる．
       */
      const int one = 1;
      setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

      int &&st =
        bind(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
      if (st != 0) {
        fprintf(stderr, "error: bind: %d\n", st);
        perror("bind");
        exit(1);
      }
    }

    /** @brief TCP 接続を受け付ける状態に入る */
    void
    listen()
    {
      int &&st = ::listen(serverfd, 50);
      if (st != 0) {
        fprintf(stderr, "error: listen: %d\n", st);
        perror("listen");
        exit(1);
      }
      /* status に LISTENING フラグを追加 */
      status |= LISTENING;
    }

    /** @brief クライアントからの TCP 接続を受理する */
    void
    accept()
    {
      len = sizeof(client_addr);
      sockfd = ::accept(serverfd, (struct sockaddr *)&client_addr, &len);
      if (sockfd < 0) {
        fprintf(stderr, "error: socket: %d\n", sockfd);
        perror("accept");
        exit(1);
      }
      /* status に INIT_SOCKET フラグを追加 */
      status |= INIT_SOCKET;
      /* status に CONNECTED フラグを追加 */
      status |= CONNECTED;
    }

  private:
    /** @brief サーバとしての socket */
    int serverfd;
    /** @brief 自身の情報を保存する構造体 */
    struct sockaddr_in server_addr;
    /** @brief 接続先クライアントの情報を受け取る構造体 */
    struct sockaddr_in client_addr;
    /** @brief client_addr の長さを受け取るための変数 */
    socklen_t len;

    void
    init_socket()
    {
      close_socket();
      serverfd = socket(AF_INET, SOCK_STREAM, 0);
      if (serverfd < 0) {
        fprintf(stderr, "error: socket: %d\n", serverfd);
        perror("socket");
        exit(1);
      }
      /* status を強制的に INIT_SERVER に変更 */
      status = INIT_SERVER;
    }
    void
    close_socket()
    {
      /* INIT_SOCKET フラグで sockfd の状態を調べる */
      if ((status & INIT_SOCKET) != 0) close(sockfd);
      /* INIT_SERVER フラグで serverfd の状態を調べる */
      if ((status & INIT_SERVER) != 0) close(serverfd);
      /* status を強制的に初期状態に変更 */
      status = INITIALIZED;
    }
  };


  /**
   * @brief クライアントとして機能するクラスを提供する
   */
  class client
    : public tcp::connection
  {
  public:
    /**
     * @brief TCP プロトコルでサーバに接続する
     * @param port_ 接続先ポート番号
     * @param ipaddr_ 接続するサーバの IP アドレス
     */
    client(const int port_, const char* ipaddr_)
      : tcp::connection(port_)
    {
      init_connection(port_, ipaddr_);
    }

    ~client()
    {
      close_socket();
    }

    /**
     * @brief 接続先サーバを設定する
     * @param port_ 接続先ポート番号
     * @param ipaddr_ 接続するサーバの IP アドレス
     */
    void
    init_connection(const int port_, const char* ipaddr_)
    {
      init_socket();
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(port);
      server_addr.sin_addr.s_addr = inet_addr(ipaddr_);
    }

    /**
     * @brief サーバとの通信を開始する
     */
    void
    connect()
    {
      int &&st =
        ::connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
      if (st != 0) {
        fprintf(stderr, "error: connect: %d\n", st);
        perror("connect");
        exit(1);
      }
      /* status に CONNECTED フラグを追加 */
      status |= CONNECTED;
    }

  private:
    /** @brief 接続先サーバの情報を保存する構造体 */
    struct sockaddr_in server_addr;

    /** @brief socket を初期化する */
    void
    init_socket()
    {
      close_socket();
      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) {
        fprintf(stderr, "error: socket: %d\n", sockfd);
        perror("socket");
        exit(1);
      }

      /*
       * 一度接続に成功すると一定時間 port が専有される．
       * SO_REUSEADDR というオプションを有効にすることで port が使われていても
       * 上書きして port に bind することができる．
       */
      const int one = 1;
      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

      /* status を強制的に INIT_SOCKET に変更 */
      status = INIT_SOCKET;
    }

    /** @brief socket が開いていたら閉じる */
    void
    close_socket()
    {
      /* INIT_SOCKET フラグで sockfd の状態を調べる */
      if ((status & INIT_SOCKET) != 0) close(sockfd);
      /* status を強制的に初期状態に変更 */
      status = INITIALIZED;
    }

  };

}
#endif
