#define REUSE_PORT 1
#define MAXPENDING 5  // Max Requests
#define BUFFERLEN 100 // recv size per iter
#define HTTPD "attohttpd"
#define URL "http://www.puyan.org"

#include <time.h>
#include <cstdio>
#include <ctype.h>
#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>  // Address Structs
#include <netinet/in.h> // IP Datatypes
#include <sys/socket.h> // Socket Functions
#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>

#define CHECK(check, message) \
  do { if ((check) < 0) { \
      fprintf(stderr, "%s failed: Error on line %d.\n", (message), __LINE__); \
      perror(message); exit(EXIT_FAILURE); \
  } } while (false)

namespace {
std::string get_mime_type(const char *name) {
  const char *dot = strrchr(name, '.');
  if (dot && !strncmp(dot, ".html", 4)) return "text/html; charset=iso-8859-1";
  if (dot && !strncmp(dot, ".midi", 4)) return "audio/midi";
  if (dot && !strncmp(dot, ".jpeg", 4)) return "image/jpeg";
  if (dot && !strncmp(dot, ".mpeg", 4)) return "video/mpeg";
  if (dot && !strncmp(dot, ".gif" , 4)) return "image/gif";
  if (dot && !strncmp(dot, ".png" , 4)) return "image/png";
  if (dot && !strncmp(dot, ".css" , 4)) return "text/css";
  if (dot && !strncmp(dot, ".au"  , 3)) return "audio/basic";
  if (dot && !strncmp(dot, ".wav" , 4)) return "audio/wav";
  if (dot && !strncmp(dot, ".avi" , 4)) return "video/x-msvideo";
  if (dot && !strncmp(dot, ".mov" , 4)) return "video/quicktime";
  if (dot && !strncmp(dot, ".mp3" , 4)) return "audio/mpeg";
  if (dot && !strncmp(dot, ".m4a" , 4)) return "audio/mp4";
  if (dot && !strncmp(dot, ".pdf" , 4)) return "application/pdf";
  if (dot && !strncmp(dot, ".ogg" , 4)) return "application/ogg";
  return "text/plain; charset=iso-8859-1";
}

std::string getTimeBuf(const struct tm *time) { // RFC1123FMT format
  if (!time) return "";
  char timebuf[100];
  strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", time);
  return std::string(timebuf);
}

void send_headers(int status, std::string title, std::string extraHeader,
                  std::string mimeType, off_t len, time_t mod, FILE *socket) {
  time_t now = time(nullptr);
  fprintf(socket, "%s %d %s\r\n", "HTTP/1.1", status, title.c_str());
  fprintf(socket, "Server: %s\r\n", HTTPD);
  fprintf(socket, "Date: %s\r\n", getTimeBuf(gmtime(&now)).c_str());
  if (extraHeader != "") fprintf(socket, "%s\r\n", extraHeader.c_str());
  if (mimeType != "") fprintf(socket, "Content-Type: %s\r\n", mimeType.c_str());
  if (len >= 0) fprintf(socket, "Content-Length: %lld\r\n", (long long)len);
  if (mod != (time_t)-1)
    fprintf(socket, "Last-Modified: %s\r\n", getTimeBuf(gmtime(&mod)).c_str());
  fprintf(socket, "Connection: close\r\n\r\n");
}

int writeHtmlEnd(int status, FILE *socket) {
  fprintf(socket, "</pre><a href=\"%s\">%s</a></body></html>\n", URL, HTTPD);
  return status;
}

int send_error(int status, std::string title, std::string extra_header,
               std::string text, FILE *socket) {
  send_headers(status, title, extra_header, "text/html", -1, -1, socket);
  fprintf(socket, "<html><head><title>%d %s</title></head>\n<body "
          "bgcolor=\"#cc9999\"><h4>%d %s</h4><pre>%s\n", status, title.c_str(),
          status, title.c_str(), text.c_str());
  return writeHtmlEnd(status, socket);
}

int doFile(const char *filename, off_t length, time_t mod, FILE *socket) {
  std::ifstream ifs(filename, std::ios::in|std::ios::binary|std::ios::ate);
  if (!ifs.is_open())
    return send_error(403, "Forbidden", "", "Protected File.", socket);

  std::streampos pos = ifs.tellg();
  char buf[static_cast<std::string::size_type>(pos)];
  bzero(buf, sizeof(buf));
  ifs.seekg(0, std::ios::beg);
  ifs.read(buf, pos);
  ifs.close();

  send_headers(200, "Ok", "", get_mime_type(filename), length, mod, socket);
  fwrite(buf, sizeof(char), sizeof(buf), socket);
  fflush(socket);
  return 200;
}

int http_proto(FILE *socket, const char *request) {
  char method[100], path[1000], protocol[100];

  if (!request || strlen(request) < strlen("GET / HTTP/1.1"))
    return send_error(403, "Bad Request", "", "No request found.", socket);
  if (sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol) != 3)
    return send_error(400, "Bad Request", "", "Can't parse request.", socket);
  if (strcasecmp(method, "get") != 0)
    return send_error(501, "Not Implemented", "", "Not implemented.", socket);
  if (path[0] != '/')
    return send_error(400, "Bad Request", "", "Bad filename.", socket);

  std::string fileStr = &(path[1]);
  if (fileStr.c_str()[0] == '\0') fileStr = "./";
  const char *file = fileStr.c_str();
  size_t len = strlen(file);
  struct stat sb;

  if (file[0] == '/' || !strcmp(file, "..") || !strncmp(file, "../", 3) ||
      strstr(file, "/../") != (char *)0 || !strcmp(&(file[len - 3]), "/.."))
    return send_error(400, "Bad Request", "", "Illegal filename.", socket);
  if (stat(file, &sb) < 0)
    return send_error(404, "Not Found", "", "File not found.", socket);

  std::string dir = std::string(file) + ((file[len - 1] != '/') ? "/" : "");
  fileStr = S_ISDIR(sb.st_mode) ? (dir + "index.html") : file;

  if (!S_ISDIR(sb.st_mode) || (stat(fileStr.c_str(), &sb) >= 0))
    return doFile(fileStr.c_str(), sb.st_size, sb.st_mtime, socket);

  send_headers(200, "Ok", "", "text/html", -1, sb.st_mtime, socket);
  fprintf(socket, "<html><head><title>Index of %s</title></head>\n<body bgcolor"
          "=\"lightblue\"><h4>Index of %s</h4><pre>", dir.c_str(), dir.c_str());
  struct dirent **dl;
  int n = scandir(dir.c_str(), &dl, NULL, alphasort);
  for (unsigned i = 0; i < n; ++i) {
    fprintf(socket, "<a href=\"%s\">%s</a>\n", dl[i]->d_name, dl[i]->d_name);
    free(dl[i]);
  }
  free(dl);
  return writeHtmlEnd(200, socket);
}

int HttpProto(int socket) {
  std::string requestStr("");
  for (char buffer[BUFFERLEN];;) {
    bzero(buffer, BUFFERLEN);
    int newBytes = read(socket, buffer, BUFFERLEN);
    requestStr += (newBytes > 0) ? std::string(buffer) : "";
    if (newBytes < BUFFERLEN) break;
  }

  FILE *socketFile = fdopen(socket, "w");
  http_proto(socketFile, requestStr.c_str());
  fclose(socketFile);
  return socket;
}

class Networking {
  private:
    const uint16_t portNumber;
    int ssock; // Server Socket

    Networking(): portNumber(0), ssock(-1) { }
    Networking(const Networking&): portNumber(0) { }

    int ContructTCPSocket() {
      int O = REUSE_PORT;                        // reuse port sockopt
      struct sockaddr_in saddr;                  // Server Socket addr
      bzero(&saddr, sizeof(saddr));              // Zero serve  struct
      saddr.sin_family = AF_INET;                // Inet addr family
      saddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming iface
      saddr.sin_port = htons(portNumber);        // Local (server) port
      CHECK(ssock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP), "socket()");
      CHECK(setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &O, sizeof(O)), "opt");
      CHECK(bind(ssock, (struct sockaddr *)&saddr, sizeof(saddr)), "bind()");
      CHECK(listen(ssock, MAXPENDING), "listen()");
      return ssock;
    }
    int ServerSocket() { return ssock >= 0 ? ssock : ContructTCPSocket(); }

  public:
    Networking(uint16_t portNumber): portNumber(portNumber), ssock(-1) { }
    int AcceptConnection() {
      struct sockaddr_in addr;
      unsigned len = sizeof(addr);
      int clientSocket = accept(ServerSocket(), (struct sockaddr *)&addr, &len);
      printf("Handling client %s\n", inet_ntoa(addr.sin_addr));
      return (clientSocket < 0) ? -1 : clientSocket;
    }
};

#define THREAD_FUNCTION(NAME) template <typename T, typename P, typename C> \
  static void NAME(SyncQueue<T, P, C> *syncQueue)

template <class T, class TP, class TC> class SyncQueue {
  private:
    void(*P)(SyncQueue<T, TP, TC>*);
    void(*C)(SyncQueue<T, TP, TC>*);
    TP ptoken;
    TC ctoken;
    const size_t maxThreads;

    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::thread> consumers;
    std::thread producer;
    std::queue<T> syncQueue;
    SyncQueue() { }
    SyncQueue(const SyncQueue&) { }

    void resizeConsumers() {
      for (int n = syncQueue.size() - consumers.size();
           (0 < n) && (consumers.size() < maxThreads); n--)
        consumers.push_back(std::move(std::thread(C, this)));
    }

  public:
    SyncQueue(void(*P)(SyncQueue<T, TP, TC>*), void(*C)(SyncQueue<T, TP, TC>*),
              TP ptoken = nullptr, TC ctoken = nullptr, size_t n = 256):
      P(P), C(C), ptoken(ptoken), ctoken(ctoken), maxThreads(n) { }

    void start() { producer = std::thread(P, this); }
    TP getProducerToken() const { return ptoken; }
    TC getConsumerToken() const { return ctoken; }

    void enqueue(T t) {
      std::unique_lock<std::mutex> lock(mutex);
      syncQueue.push(t);
      resizeConsumers();
      cv.notify_all();
    }

    T dequeue() {
      std::unique_lock<std::mutex> lock(mutex);
      while (!syncQueue.size()) cv.wait(lock);
      T t = syncQueue.front();
      syncQueue.pop();
      return t;
    }
};
} // end anonymous namespace

THREAD_FUNCTION(producer) {
  for (printf("[PRODUCER] Listening\n");;) {
    printf("[PRODUCER] Waiting to accept a connection\n");
    syncQueue->enqueue(syncQueue->getProducerToken()->AcceptConnection());
  }
}

THREAD_FUNCTION(consumer) {
  for (int S = -1;;S = syncQueue->dequeue()) {
    printf("[CONSUMER] Handling Socket %d.\n", S);
    if (S >= 0) close(HttpProto(S));
  }
}

int main(int argc, char **argv) {
  std::string LOAD_DIR = ".";
  printf("* Beginning Server, Root Directory: %s\n", LOAD_DIR.c_str());
  CHECK(chdir(LOAD_DIR.c_str()), "Error setting home directory.\n");
  Networking N(1337);
  SyncQueue<int, Networking*, void*> sQueue(producer, consumer, &N, nullptr);
  sQueue.start();
  while(getchar() != 'q') printf("* Server Started, Enter q to Quit *\n\n");
  return EXIT_SUCCESS;
}
