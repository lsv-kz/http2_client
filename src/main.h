#ifndef CLIENT_H_
#define CLIENT_H_
#define _FILE_OFFSET_BITS 64
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>

#include <mutex>
#include <thread>
#include <condition_variable>

#include <signal.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "bytes_array.h"
#include "huffman.h"

const int  ERR_TRY_AGAIN = -1000;
const unsigned char alpn[] = {2, 'h', '2'};

extern char Host[256];
extern char IP[256];
extern int ai_family;
extern char Uri[1024];
extern char Method[16];
extern char PostData[512];
extern char Range[64];

extern const char *static_tab[][2];

enum OPERATION_TYPE { CONNECT = 1, SSL_CONNECT, PREFACE_MESSAGE, SEND_SETTINGS, WORK_STREAM, };

enum FRAME_TYPE 
{
    DATA,
    HEADERS,
    PRIORITY,
    RST_STREAM,
    SETTINGS,
    PUSH_PROMISE,
    PING, 
    GOAWAY,
    WINDOW_UPDATE,
    CONTINUATION,
    ALTSVC,
    ORIGIN = 0x0C,
    CACHE_DIGEST = 0x0D,
    PRIORITY_UPDATE = 0x10
};

enum HTTP2_FLAGS
{
    FLAG_ACK = 0x1,
    FLAG_END_STREAM = 0x1,
    FLAG_END_HEADERS = 0x4,
    FLAG_PADDED = 0x8,
    FLAG_PRIORITY = 0x20
};

enum HTTP2_ERRORS
{
    NO_ERROR,
    PROTOCOL_ERROR,
    INTERNAL_ERROR,
    FLOW_CONTROL_ERROR,
    SETTINGS_TIMEOUT,
    STREAM_CLOSED,
    FRAME_SIZE_ERROR,
    REFUSED_STREAM,
    CANCEL,
    COMPRESSION_ERROR,
    CONNECT_ERROR,
    ENHANCE_YOUR_CALM,
    INADEQUATE_SECURITY,
    HTTP_1_1_REQUIRED,
};

void hex_print_stderr(const char *s, int line, const void *p, int n);
//======================================================================
struct Config
{
    SSL_CTX *ctx;
    int num_connections;
    int num_req;

    std::string RequestsPath;
    std::string LogPath;

    long SettingsHeaderTableSize;  // 1
    long MaxConcurrentStreams;     // 3
    long InitialWindowSize;       // 4
    long SettingsMaxFrameSize;     // 5

    long MinWindowSize;
    long MaxWindowSize;

    int Timeout;
    int TimeoutPoll;
    char ip[256];
    char port[32];
    char UserAgent[128];
    const char *req;
    int (*create_sock)(const char*, const char*, int*);

    void init()
    {
        MaxConcurrentStreams = 128;
        InitialWindowSize = -1;
        Timeout = 30;
        TimeoutPoll = 10;
        snprintf(UserAgent, sizeof(UserAgent), "anonymous");
    }
};
//======================================================================
extern const Config* const conf;
//======================================================================
struct Stream
{
    Stream *prev;
    Stream *next;

    unsigned long num_conn;

    int id;

    ByteArray headers;
    ByteArray frame_win_update;
    long serv_stream_window_size;
    long long recv_bytes;

    Stream()
    {
        if (conf->InitialWindowSize >= 0)
            serv_stream_window_size = conf->InitialWindowSize;
        else
            serv_stream_window_size = 65535;
        id = 0;
        recv_bytes = 0;
        num_conn = 0;
    }

    ~Stream()
    {
        //fprintf(stderr, "[%lu/%d]<~~~> serv_stream_window_size=%ld\n", num_conn, id, serv_stream_window_size);
    }
};
//======================================================================
struct Header
{
    char *name;
    char *val;
    int size;
};
//----------------------------------------------------------------------
class DynamicTable
{
    Header *table;
    
    int max_table_size;
    int table_size;

    int max_headers_num;
    int headers_num;
    
    int offset;
    int err;

    DynamicTable();
    DynamicTable(const DynamicTable&);
    DynamicTable& operator= (const DynamicTable&);

public:

    DynamicTable(int size_, int offs)
    {
        max_table_size = size_;
        max_headers_num = max_table_size/20;
        table_size = 0;
        offset = offs;
        headers_num = err = 0;
        table = new(std::nothrow) Header [max_table_size];
        if (!table)
        {
            fprintf(stderr, "<%s:%d> Error: %s\n", __func__, __LINE__, strerror(errno));
            max_table_size = 0;
            err = 1;
            return;
        }
        //fprintf(stderr, "<%s:%d> table_size=%d, max_headers_num=%d, offset=%d\n", __func__, __LINE__, max_table_size, max_headers_num, offset);
        table[0].name = NULL;
        table[0].val = NULL;
    }
    //------------------------------------------------------------------
    ~DynamicTable()
    {
        if (table)
        {
            //fprintf(stderr, "<%s:%d> ~~~ Delete Dynamic Table\n", __func__, __LINE__);
            for ( int i = 0; i < headers_num; ++i)
            {
                if (table[i].name && table[i].val)
                {
                    delete [] table[i].name;
                }
            }
            delete [] table;
            table = NULL;
        }
    }
    //------------------------------------------------------------------
    int add(std::string& name, std::string& val);
    //------------------------------------------------------------------
    void print()
    {
        fprintf(stderr, " -------- Dynamic table %d, size %d --------\n", headers_num, table_size);
        for ( int i = 0; i < headers_num; ++i)
        {
            fprintf(stderr, " %04d  [%s: %s]\n", i + offset, table[i].name, table[i].val);
        }
    }
    //------------------------------------------------------------------
    Header *get(int n)
    {
        if ((n < offset) || (n >= (headers_num + offset)))
        {
            fprintf(stderr, "<%s:%d> Error out of range: index=%d, table_len=%d, offset=%d\n", __func__, __LINE__, n, headers_num, offset);
            return NULL;
        }

        return &table[n - offset];
    }
};
//======================================================================
struct Connect
{
    Connect *prev;
    Connect *next;

    unsigned long num_conn;

    Stream **req_array;
    int index_req;
    
    ByteArray *frame_try_again;

    SSL *ssl;
    int ssl_err;
    int ssl_pending;

    OPERATION_TYPE operation;

    int max_req;
    int num_close_stream;
    int num_work_stream;
    int stream_id;

    int servSocket;

    time_t sock_timer;
    int timeout;
    int events;
    int revents;

    bool recv_settings_ack;
    bool send_settings_ack;

    char frame_head[9];
    int len_frame_head;

    FRAME_TYPE frame_type;
    int frame_id;
    int frame_size;
    int frame_flags;

    ByteArray payload;
    ByteArray settings;
    ByteArray headers;
    ByteArray data;
    ByteArray frame_win_update;

    bool send_goaway;
    bool send_headers;

    bool print_entity;
    bool print_all_headers;

    long serv_connect_window_size;

    long long  read_bytes;

    HuffmanCode huff;

    DynamicTable *dyn_tab;

    Connect()
    {
        req_array = NULL;
        frame_try_again = NULL;
        index_req = 0;
        max_req = 1;
        num_close_stream = 0;
        num_conn = stream_id = 1;
        ssl = NULL;
        ssl_err = 0;
        num_work_stream = 0;

        serv_connect_window_size = 65535;

        sock_timer = 0;
        next = NULL;
        len_frame_head = frame_size = 0;
        read_bytes = 0;
        recv_settings_ack = send_settings_ack = false;
        send_goaway = send_headers = false;
        print_entity = false;
        print_all_headers = true;

        dyn_tab = NULL;
        set_frame_settings();
    }

    ~Connect()
    {
        //fprintf(stderr, "[%lu]<~> serv_connect_window_size=%ld\n", num_conn, serv_connect_window_size);
        if (req_array)
        {
            for (int i = 0; i < max_req; ++i)
            {
                Stream *r = req_array[i];
                if (r)
                {
                    //fprintf(stderr, "[%lu/%d]<~~~> serv_stream_window_size=%ld\n", num_conn, r->id, r->serv_stream_window_size);
                    delete r;
                }
            }
            
            delete [] req_array;
            req_array = NULL;
        }
    }

    Stream *new_stream(int id)
    {
        Stream *req = NULL;
        req = new(std::nothrow) Stream;
        if (!req)
        {
            fprintf(stderr, "<%s:%d> Error: %s\n", __func__, __LINE__, strerror(errno));
            return NULL;
        }

        req->id = id;
        req->num_conn = num_conn;

        return req;
    }

    void set_bytes(char *s, int d, int i)
    {
        int shift = 24;
        for ( ; shift >= 0; )
        {
            *(s + i) = (d>>shift);
            ++i;
            shift -= 8;
        }

        s[i - 4] = s[i - 4] & 0x7f;
    }
    
    void set_frame_settings()
    {
        char s[32] = "\x00\x00\x00\x04\x00\x00\x00\x00\x00";  // SETTINGS (type=0x4)
        int frame_size = 9;

        if (conf->SettingsHeaderTableSize >= 0)
        {
            // SETTINGS_HEADER_TABLE_SIZE (0x1)
            memcpy(s + frame_size, "\x00\x01\x00\x00\x00\x00", 6);
            frame_size += 6;
            set_bytes(s, conf->SettingsHeaderTableSize, frame_size - 4);
        }

        if (conf->SettingsHeaderTableSize > 0)
        {
            dyn_tab = new(std::nothrow) DynamicTable(conf->SettingsHeaderTableSize, 62);
            if (!dyn_tab)
            {
                fprintf(stderr, "<%s:%d>Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
                exit(1);
            }
        }
        else if (conf->SettingsHeaderTableSize == 0)
        {
            dyn_tab = NULL;
        }
        else
        {
            dyn_tab = new(std::nothrow) DynamicTable(4096, 62);
            if (!dyn_tab)
            {
                fprintf(stderr, "<%s:%d>Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
                exit(1);
            }
    //fprintf(stderr, "<%s:%d> dyn_tab: %p, 4096 bytes\n", __func__, __LINE__, dyn_tab);
        }

        // SETTINGS_MAX_CONCURRENT_STREAMS (0x3)
        memcpy(s + frame_size, "\x00\x03\x00\x00\x00\x00", 6);
        frame_size += 6;
        if (conf->MaxConcurrentStreams > 0)
            set_bytes(s, conf->MaxConcurrentStreams, frame_size - 4);
        else
            set_bytes(s, 128, frame_size - 4);

        if (conf->InitialWindowSize >= 0)
        {
            // SETTINGS_INITIAL_WINDOW_SIZE (0x4)
            memcpy(s + frame_size, "\x00\x04\x00\x00\x00\x00", 6);
            frame_size += 6;
            set_bytes(s, conf->InitialWindowSize, frame_size - 4);
        }

        if (conf->SettingsMaxFrameSize > 0)
        {
            // SETTINGS_MAX_FRAME_SIZE (0x5)
            memcpy(s + frame_size, "\x00\x05\x00\x00\x00\x00", 6);
            frame_size += 6;
            set_bytes(s, conf->SettingsMaxFrameSize, frame_size - 4);
        }

        s[2] = frame_size - 9;
//hex_print_stderr("set SEND_SETTINGS", __LINE__, s, frame_size);
        settings.cpy(s, frame_size);
    }
};
//======================================================================
void set_max_concurrent_streams(int n);
//======================================================================
int create_connections();
void create_frame_headers(Connect *c);
int send_frame_window_update(Connect *c, Stream *req);
int get_window_update_size(const char *s);
//======================================================================
void std_in(char *s, int len);
const char *strstr_case(const char *s1, const char *s2);
int strcmp_case(const char *s1, const char *s2);
int strlcmp_case(const char *s1, const char *s2, int len);
const char *get_str_operation(OPERATION_TYPE n);
const char *get_str_frame_type(FRAME_TYPE t);
const char *get_str_setting_param(int n);
const char *get_http2_error(int err);
int read_req_file(const char *path);
void set_id(ByteArray *ba, int d);
void set_frame_headers(Connect *conn);
void add_header(Connect *conn, int ind);
void add_header(Connect *conn, int ind, const char *val);
void add_header(Connect *conn, const char *name, const char *val);
void set_frame_data(Connect *conn, const char *data, int size, int flag);
int set_frame_window_update(Stream *con, unsigned int size);
int set_frame_window_update(Connect *con, unsigned int size);
int parse_headers(Connect *conn);
int parse_frame_headers(Connect *c);
int create_log_file(const char *);
//======================================================================
int create_client_socket(const char *host, const char *port);
int create_client_socket_ip4(const char *ip, const char *port, int*);
int create_client_socket_ip6(const char *ip, const char *port, int*);
int get_ip(int sock, char *ip, int size_ip);
const char *get_str_ai_family(int ai_family);
int write_to_client(Connect *c, const char *buf, int len);
int read_from_client(Connect *c, char *buf, int len);
//======================================================================
SSL_CTX* InitCTX();
int ctx_set_alpn_protos(SSL_CTX *ctx, const unsigned char *alpn, unsigned int size);
const char *ssl_strerror(int err);
int ssl_read(Connect *con, char *buf, int len);
int ssl_write(Connect *con, const char *buf, int len);

#endif
