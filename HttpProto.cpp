#include "atto.h"

#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {

std::string get_mime_type(const char *name) {
  if (const char *dot = strrchr(name, '.')) {
    if (!strcmp(dot, ".html")) return "text/html; charset=iso-8859-1";
    if (!strcmp(dot, ".htm"))  return "text/html; charset=iso-8859-1";
    if (!strcmp(dot, ".jpg"))  return "image/jpeg";
    if (!strcmp(dot, ".jpeg")) return "image/jpeg";
    if (!strcmp(dot, ".gif"))  return "image/gif";
    if (!strcmp(dot, ".png"))  return "image/png";
    if (!strcmp(dot, ".css"))  return "text/css";
    if (!strcmp(dot, ".au"))   return "audio/basic";
    if (!strcmp(dot, ".wav"))  return "audio/wav";
    if (!strcmp(dot, ".avi"))  return "video/x-msvideo";
    if (!strcmp(dot, ".mov"))  return "video/quicktime";
    if (!strcmp(dot, ".qt"))   return "video/quicktime";
    if (!strcmp(dot, ".mpeg")) return "video/mpeg";
    if (!strcmp(dot, ".mpe"))  return "video/mpeg";
    if (!strcmp(dot, ".vrml")) return "model/vrml";
    if (!strcmp(dot, ".wrl"))  return "model/vrml";
    if (!strcmp(dot, ".midi")) return "audio/midi";
    if (!strcmp(dot, ".mid"))  return "audio/midi";
    if (!strcmp(dot, ".mp3"))  return "audio/mpeg";
    if (!strcmp(dot, ".m4a"))  return "audio/mp4";
    if (!strcmp(dot, ".pdf"))  return "application/pdf";
    if (!strcmp(dot, ".ogg"))  return "application/ogg";
    if (!strcmp(dot, ".pac"))  return "application/x-ns-proxy-autoconfig";
  }

  return "text/plain; charset=iso-8859-1";
}

int hexit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;

  assert(false && "Shouldn't happen, we're guarded by isxdigit()");
  return 0;
}

void strdecode(char *to, char *from) {

  for (; *from != '\0'; ++to, ++from) {
    if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
      *to = hexit(from[1]) * 16 + hexit(from[2]);
      from += 2;
    } else {
      *to = *from;
    }
  }

  *to = '\0';
}

void strencode(char *to, size_t tosize, const char *from) {
  int tolen;

  for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {
    if (isalnum(*from) || strchr("/_.-~", *from) != (char *)0) {
      *to = *from;
      ++to;
      ++tolen;
    } else {
      sprintf(to, "%%%02x", (int)*from & 0xff);
      to += 3;
      tolen += 3;
    }
  }

  *to = '\0';
}

std::string getTimeBuf(const struct tm *time) {
  if (!time) return nullptr;
  char timebuf[100];
  #define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
  strftime(timebuf, sizeof(timebuf), RFC1123FMT, time);
  std::string timeStr = std::string(timebuf);
  return timeStr;
}

void file_details(FILE *socket, const char *dir, const char *name) {
  char path[strlen(dir) + strlen(name) + 2 /* '/' + '\0' */];
  sprintf(path, "%s/%s", dir, name);

  struct stat sb;
  if (lstat(path, &sb) < 0) {
    fprintf(socket, "<a href=\"%s\">%s</a>INVALID PATH\n", name, name);
    return;
  }

  char encodeName[1000];
  strencode(encodeName, sizeof(encodeName), name);
  std::string timeStr = getTimeBuf(localtime(&sb.st_mtime));
  fprintf(socket, "<a href=\"%s\">%-32.32s</a> %15s %14lld\n", encodeName, name,
          timeStr.c_str(), (long long)sb.st_size);
}

void send_headers(int status, std::string title, std::string extraHeader,
                  std::string mimeType, off_t len, time_t mod, FILE *socket) {
  time_t now = time(nullptr);
  fprintf(socket, "%s %d %s\r\n", "HTTP/1.1", status, title.c_str());
  fprintf(socket, "Server: %s\r\n", SERVER_NAME);
  fprintf(socket, "Date: %s\r\n", getTimeBuf(gmtime(&now)).c_str());
  if (extraHeader != "")
    fprintf(socket, "%s\r\n", extraHeader.c_str());
  if (mimeType != "")
    fprintf(socket, "Content-Type: %s\r\n", mimeType.c_str());
  if (len >= 0)
    fprintf(socket, "Content-Length: %lld\r\n", (long long)len);
  if (mod != (time_t)-1)
    fprintf(socket, "Last-Modified: %s\r\n", getTimeBuf(gmtime(&mod)).c_str());
  fprintf(socket, "Connection: close\r\n\r\n");
}

int send_error(int status, std::string title, std::string extra_header,
               std::string text, FILE *socket) {
  send_headers(status, title, extra_header, "text/html", -1, -1, socket);
  fprintf(socket, "<html><head><title>%d %s</title></head>\n<body "
          "bgcolor=\"#cc9999\"><h4>%d %s</h4>\n", status, title.c_str(), status,
          title.c_str());
  fprintf(socket, "%s\n", text.c_str());
  fprintf(socket, "<hr>\n<address><a href=\"%s\">%s</a></address>\n",
          SERVER_URL, SERVER_NAME);
  fprintf(socket, "</body></html>\n");
  return status;
}

int doFile(const char *filename, off_t length, time_t mod, FILE *socket) {
  FILE *fp = fopen(filename, "r");
  if (!fp) return send_error(403, "Forbidden", "", "Protected File.", socket);
  send_headers(200, "Ok", "", get_mime_type(filename), length, mod, socket);

  int ich;
  while (fp && (ich = getc(fp)) != EOF)
    fprintf(socket, "%c", ich);

  fclose(fp);
  return 200;
}

int http_proto(FILE *socket, const char *request) {
  char method[100], path[1000], protocol[100];
  struct stat sb;

  if (!request || strlen(request) < strlen("GET / HTTP/1.1"))
    return send_error(403, "Bad Request", "", "No request found.", socket);
  if (sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol) != 3)
    return send_error(400, "Bad Request", "", "Can't parse request.", socket);
  if (strcasecmp(method, "get") != 0)
    return send_error(501, "Not Implemented", "", "Not implemented.", socket);
  if (path[0] != '/')
    return send_error(400, "Bad Request", "", "Bad filename.", socket);

  char *file = &(path[1]);
  strdecode(file, file);

  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wwritable-strings"
  if (file[0] == '\0')
    file = "./";
  #pragma clang diagnostic pop

  size_t len = strlen(file);

  if (file[0] == '/' || strcmp(file, "..") == 0 ||
      strncmp(file, "../", 3) == 0 || strstr(file, "/../") != (char *)0 ||
      strcmp(&(file[len - 3]), "/..") == 0)
    return send_error(400, "Bad Request", "", "Illegal filename.", socket);

  if (stat(file, &sb) < 0)
    return send_error(404, "Not Found", "", "File not found.", socket);

  if (S_ISDIR(sb.st_mode)) {

    char location[strlen(file) + 2 /* '/' + '\0' */];
    if (file[len - 1] != '/') {
      sprintf(location, "%s/", file);
      file = location;
    }

    char idx[strlen(file) + strlen("index.html") + 1];
    sprintf(idx, "%sindex.html", file);
    if (stat(idx, &sb) >= 0)
      return doFile(idx /* filename */, sb.st_size, sb.st_mtime, socket);

    send_headers(200, "Ok", "", "text/html", -1, sb.st_mtime, socket);

    fprintf(socket, "<html><head><title>Index of %s</title></head>\n<body "
            "bgcolor=\"lightblue\"><h4>Index of %s</h4>\n<pre>\n",
            file, file);

    struct dirent **dl;
    int n = scandir(file, &dl, NULL, alphasort);
    if (n < 0)
      perror("scandir");
    for (unsigned i = 0; i < n; ++i) {
      file_details(socket, file, dl[i]->d_name);
      free(dl[i]);
    }

    free(dl);

    fprintf(socket, "</pre>\n<hr>\n<address>"
            "<a href=\"%s\">%s</a></address>\n</body></html>\n",
            SERVER_URL, SERVER_NAME);
    return 200;
  }

  return doFile(file, sb.st_size, sb.st_mtime, socket);
}

} // end anonymous namespace

int HttpProtoWrapper(int socket, const char *request) {
  FILE *socketFile = fdopen(socket, "w");
  fflush(socketFile);
  int result = http_proto(socketFile, request);
  fflush(socketFile);
  fclose(socketFile);
  return result;
}

