/**
 * @file tcp.h
 * @brief Providing classis to communicate with sockets
 * @author Ryou OHSAWA
 * @date 2015
 *
 * This header provides classes for transactions in the TCP protocol.
 *
 * The following classes are provided in this file:
 * - server: host a server in the TCP protocol.
 * - client: communicate with a server in the TCP protocol.
 *
 * An IP address and a port number are given in a string and an integer.
 *
 * Reciever classes use a function `read(buf, N)` for getting data, where
 * `buf` is a buffer to store data and `N` is the maximum number of data
 * to be read in units of bytes. Transmitters use a function `send(data, N)`
 * for submitting data, where `data` contains data to be submitted and `N`
 * is the number of data submitted in units of bytes.
 */
#ifndef __H_CONNECTION_TCP
#define __H_CONNECTION_TCP

#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/**
 * @brief Defining classes for communication in the TCP protocol.
 */
namespace tcp
{
  /** A default buffer size in bytes */
  constexpr size_t BUFSIZE = 2880;

  /**
   * @brief Defining the status of connections.
   */
  enum tcp_status {
    INITIALIZED = 0x0000,    /**< @brief Initialized */
    INIT_AS_SERVER = 0x0001, /**< @brief Initialized as a server */
    INIT_AS_CLIENT = 0x0002, /**< @brief Initialized as a client */
    OPEN_SOCKET = 0x0010,    /**< @brief A sockfd is opened */
    OPEN_SERVER = 0x0020,    /**< @brief A serverfd is opened */
    LISTENING   = 0x0200,    /**< @brief A server is now listening */
    CONNECTED   = 0x1000,    /**< @brief Connection established */
  };


  /**
   * @brief A base class for TCP communication.
   * @note This is a virtual class.
   */
  class connection
  {
  public:
    /**
     * @brief Create an instance with `_port` and `_ipaddr`.
     * @param[in] _port A port number.
     * @param[in] _ipaddr An IP address.
     */
    connection(const int32_t _port, const char* _ipaddr)
    {
      port   = _port;
      ipaddr = (_ipaddr==NULL)?"127.0.0.1":std::string(_ipaddr);
      status = tcp_status::INITIALIZED;
    }

    ~connection() {
      close_sockfd();
      close_serverfd();
    }

    /**
     * @brief Return the current port number.
     * @return The current port number.
     */
    const int32_t
    get_port() const {
      return port;
    }

    /**
     * @brief Return the current socket descriptor.
     * @return The current socket descriptor.
     * @note `-1` returned if the socket is not opened.
     */
    const int32_t
    get_socket() const {
      if (status&tcp_status::OPEN_SOCKET) return sockfd;
      return -1;
    }

    /**
     * @brief Return the current server socket descriptor.
     * @return The current server socket descriptor.
     * @note `-1` returned if the server is not opened.
     */
    const int32_t
    get_server_socket() const {
      if (status&tcp_status::OPEN_SOCKET) return serverfd;
      return -1;
    }

    /**
     * @brief Recieve data.
     * @param[in] buf A buffer to store data.
     * @param[in] n The size of data to be read in bytes.
     * @return The size of data acutually read in bytes.
     */
    template <class T>
    const int32_t read(T* buf, const int32_t n)
    {
      return read((void*)buf, n);
    }
    /**
     * @brief Recieve data.
     * @param[in] buf A buffer to store data.
     * @param[in] n The size of data to be read in bytes.
     * @return The size of data acutually read in bytes.
     */
    const int32_t
    read(void* buf, const int32_t n)
    {
      if ((status & tcp_status::CONNECTED) == 0) {
        fprintf(stderr, "error: not connected yet.\n");
        return -1;
      }
      int32_t &&num = ::read(sockfd, buf, n);
      return num;
    };

    /**
     * @brief Recieve data in the non-blocking mode.
     * @param[in] buf A buffer to store data.
     * @param[in] n The size of data to be read in bytes.
     * @return The size of data acutually read in bytes.
     * @note Immediately return if there is no data in the IO buffer.
     */
    template <class T>
    const int32_t
    partial_read(T* buf, const int32_t n)
    {
      set_nonblock();
      int32_t &&num = read((void*)buf,n);
      unset_nonblock();
      return num;
    }

    /**
     * @brief Submit data.
     * @param[out] data A buffer to sotre data to be submitted.
     * @param[in] n The size of data in bytes.
     * @return The size of data acutually submitted.
     */
    template <class T>
    const int32_t
    write(const T* data, const int32_t n)
    {
      return write((const void*)data, n);
    }
    /**
     * @brief Submit data.
     * @param[out] data A buffer to sotre data to be submitted.
     * @param[in] n The size of data in bytes.
     * @return The size of data acutually submitted.
     */
    const int32_t
    write(const void* data, const int32_t n)
    {
      if ((status & tcp_status::CONNECTED) == 0) {
        fprintf(stderr, "error: not connected yet.\n");
        return -1;
      }
      int32_t &&num = ::write(sockfd, data, n);
      return num;
    }

    /**
     * @brief Close a connection.
     * @return Zero if successful. `-1` if connection is not established.
     */
    int32_t
    close()
    {
      if (status&tcp_status::CONNECTED) {
        close_sockfd();
        return 0;
      } else {
        return -1;
      }
    }

    /**
     * @brief Enable the non-blocking mode.
     */
    void
    set_nonblock()
    {
      int32_t flag = fcntl(sockfd, F_GETFL, 0);
      fcntl(sockfd, F_SETFL, flag |= O_NONBLOCK);
    }

    /**
     * @brief Disable the non-blocking mode.
     */
    void
    unset_nonblock()
    {
      int32_t flag = fcntl(sockfd, F_GETFL, 0);
      fcntl(sockfd, F_SETFL, flag &= ~O_NONBLOCK);
    }

  protected:
    int32_t sockfd;     /**< A socket for communication */
    int32_t serverfd;   /**< A socket to host a server */
    int32_t port;       /**< A port number */
    std::string ipaddr; /**< An IP address */
    uint32_t status;    /**< Store the status of the instance */

    /** Close `sockfd` when the socket is opened. */
    void
    close_sockfd()
    {
      if (status & tcp_status::OPEN_SOCKET) ::close(sockfd);
      status |= ~tcp_status::OPEN_SOCKET;
    }

    /** Close `serverfd` when opened. */
    void
    close_serverfd()
    {
      if (status & tcp_status::OPEN_SERVER) ::close(serverfd);
      status |= ~tcp_status::OPEN_SERVER;
    }
  private:
  };


  /**
   * @brief A class to host a TCP server.
   */
  class server
    : public tcp::connection
  {
  public:
    /**
     * @brief Create a server instance with `_port` and `_ipaddr`.
     * @param[in] _port A port number to be opened.
     * @param[in] _ipaddr A network to host a server.
     * @note `127.0.0.1` is set if `_ipaddr` is not specified.
     * When a blank `_ipaddr` is given, `INADDR_ANY` is set.
     */
    server(const int32_t _port, const char* _ipaddr = NULL)
      : tcp::connection(_port, _ipaddr)
    { init_server(); }

    /**
     * @brief Start listening to a TCP connection from clients.
     * @note A flag tcp::tcp_status::LISTENING is turned on.
     */
    void
    listen()
    {
      int32_t &&st = ::listen(serverfd, 50);
      if (st != 0) {
        fprintf(stderr, "error: listen: %d\n", st);
        perror("listen");
        exit(1);
      }
      status |= tcp_status::LISTENING;
    }

    /**
     * @brief Accept a TCP connection from a client.
     * @note A flag tcp::tcp_status::OPEN_SOCKET is turned on.
     * @note A flag tcp::tcp_status::CONNECTED is turned on.
     */
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
      status |= tcp_status::OPEN_SOCKET;
      status |= tcp_status::CONNECTED;
    }

  private:
    struct sockaddr_in server_addr; /**< store server information */
    struct sockaddr_in client_addr; /**< store client information */
    socklen_t len; /**< store the length of a client_addr */

    /**
     * @brief Initialize the intance as a TCP server.
     * @param[in] _port A port number to be opened.
     * @param[in] _ipaddr A network to host a server.
     * @note A flag tcp::tcp_status::INIT_AS_SERVER is turned on.
     * @note A flag tcp::tcp_status::OPEN_SERVER is turned on.
     */
    void
    init_server()
    {
      close_serverfd();
      serverfd = socket(AF_INET, SOCK_STREAM, 0);
      if (serverfd < 0) {
        fprintf(stderr, "error: socket: %d\n", serverfd);
        perror("socket");
        exit(1);
      }
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(port);
      if (ipaddr == "") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
      } else {
        server_addr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
      }

      /*
       * A port number is occupied for a while after a connection is
       * established. If `SO_REUSEADDR` option is enabled, we can bind
       * a port which is already used in previous connections.
       */
      const int32_t one = 1;
      setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int32_t));

      int32_t &&st =
        bind(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
      if (st != 0) {
        fprintf(stderr, "error: bind: %d\n", st);
        perror("bind");
        exit(1);
      }

      status |= tcp_status::INIT_AS_SERVER;
      status |= tcp_status::OPEN_SERVER;
    }
  };


  /**
   * @brief A class to communicate with a server as a client.
   */
  class client
    : public tcp::connection
  {
  public:
    /**
     * @brief Create a client instance to a server with `_port` and `_ipaddr`.
     * @param _port A port number of the server.
     * @param _ipaddr An IP address of the server.
     */
    client(const int32_t _port, const char* _ipaddr)
      : tcp::connection(_port, _ipaddr)
    {
      init_connection();
    }

    /**
     * @brief Establish a connection with the server.
     * @note A flag tcp::tcp_status::CONNECTED is turned on.
     */
    void
    connect()
    {
      int32_t &&st =
        ::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
      if (st != 0) {
        fprintf(stderr, "error: connect: %d\n", st);
        perror("connect");
        exit(1);
      }
      status |= tcp_status::CONNECTED;
    }

  private:
    struct sockaddr_in server_addr; /**< Store server information */

    /**
     * @brief Configure a connection
     * @note A flat tcp::tcp_status::OPEN_SOCKET is turned on.
     */
    void
    init_connection()
    {
      close_sockfd();

      /*
       * A port number is occupied for a while after a connection is
       * established. If `SO_REUSEADDR` option is enabled, we can bind
       * a port which is already used in previous connections.
       */
      const int32_t one = 1;
      setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int32_t));

      sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0) {
        fprintf(stderr, "error: socket: %d\n", sockfd);
        perror("socket");
        exit(1);
      }

      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(port);
      server_addr.sin_addr.s_addr = inet_addr(ipaddr.c_str());

      status |= tcp_status::INIT_AS_CLIENT;
      status |= tcp_status::OPEN_SOCKET;
    }
  };

}
#endif
