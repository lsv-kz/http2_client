#include "main.h"

using namespace std;
//======================================================================
char IP[256];
int ai_family;

char Method[16];
char Uri[1024];
char Host[256] = "0.0.0.0";
char Range[64];
char PostData[512];

static Config c;
const Config* const conf = &c;

void print_line();
//======================================================================
static void signal_handler(int signo)
{
    if (signo == SIGINT)
    {
        fprintf(stderr, "<%s> ####### SIGINT #######\n", __func__);
        exit(1);
    }
    else if (signo == SIGSEGV)
    {
        fprintf(stderr, "<%s> ####### SIGSEGV #######\n", __func__);
        abort();
    }
    else
        fprintf(stderr, "<%s> ? signo=%d (%s)\n",  __func__, signo, strsignal(signo));
}
//======================================================================
void get_time_connect(struct timeval *time1, char *buf, int size_buf)
{
    unsigned long ts12, tu12;
    struct timeval time2;

    gettimeofday(&time2, NULL);

    if ((time2.tv_usec-time1->tv_usec) < 0)
    {
        tu12 = (1000000 + time2.tv_usec) - time1->tv_usec;
        ts12 = (time2.tv_sec - time1->tv_sec) - 1;
    }
    else
    {
        tu12 = time2.tv_usec - time1->tv_usec;
        ts12 = time2.tv_sec - time1->tv_sec;
    }

    snprintf(buf, size_buf, "Time: %lu.%06lu sec", ts12, tu12);
}
//======================================================================
void set_max_concurrent_streams(int n)
{
    c.MaxConcurrentStreams = n;
}
//======================================================================
int is_number(const char *s)
{
    if (!s)
        return 0;
    int n = isdigit((int)*(s++));
    while (*s && n)
        n = isdigit((int)*(s++));
    return n;
}
//======================================================================
int read_conf_file()
{
    char *p1, *p2, s[256];
    FILE *f = fopen("config.txt", "r");
    if (!f)
    {
        printf(" Error open config file: %s\n", strerror(errno));
        return -1;
    }

    c.init();

    while (fgets(s,sizeof(s), f))
    {
        if ((p1 = strpbrk(s, "\r\n")))
            *p1 = 0;
        p1 = s;

        while ((*p1 == ' ') || (*p1 == '\t'))
        {
            p1++;
        }

        if (*p1 == '#' || *p1 == 0)
            continue;
        else
        {
            if ((p2 = strchr(s, '#')))
                *p2 = 0;
        }

        if (!strncmp(p1, "RequestsPath", 12))
        {
            c.RequestsPath = "";
            p1 += 12;
            for ( ; *p1; )
            {
                char ch = *p1++;
                if ((ch != ' ') && (ch != '\t'))
                {
                    c.RequestsPath += ch;
                }
            }
            printf("RequestsPath: %s\n", c.RequestsPath.c_str());
            continue;
        }
        else if (!strncmp(p1, "LogPath", 7))
        {
            c.LogPath = "";
            p1 += 7;
            for ( ; *p1; )
            {
                char ch = *p1++;
                if ((ch != ' ') && (ch != '\t'))
                {
                    c.LogPath += ch;
                }
            }
            printf("LogPath: %s\n", c.LogPath.c_str());
            continue;
        }
        else if (sscanf(p1, " MaxConcurrentStreams %ld", &c.MaxConcurrentStreams) == 1)
        {
            printf("MaxConcurrentStreams: %ld\n", c.MaxConcurrentStreams);
            continue;
        }
        else if (sscanf(p1, " SettingsHeaderTableSize %ld", &c.SettingsHeaderTableSize) == 1)
        {
            printf("SettingsHeaderTableSize: %ld\n", c.SettingsHeaderTableSize);
            continue;
        }
        else if (sscanf(p1, " InitialWindowSize %ld", &c.InitialWindowSize) == 1)
        {
            printf("InitialWindowSize: %ld\n", c.InitialWindowSize);
            continue;
        }
        else if (sscanf(p1, " SettingsMaxFrameSize %ld", &c.SettingsMaxFrameSize) == 1)
        {
            printf("SettingsMaxFrameSize: %ld\n", c.SettingsMaxFrameSize);
            continue;
        }
        else if (sscanf(p1, " MaxWindowSize %ld", &c.MaxWindowSize) == 1)
        {
            printf("MaxWindowSize: %ld\n", c.MaxWindowSize);
            continue;
        }
        else if (sscanf(p1, " MinWindowSize %ld", &c.MinWindowSize) == 1)
        {
            printf("MinWindowSize: %ld\n", c.MinWindowSize);
            continue;
        }
        else if (sscanf(p1, " Timeout %d", &c.Timeout) == 1)
        {
            printf("Timeout: %d s\n", c.Timeout);
            continue;
        }
        else if (sscanf(p1, " TimeoutPoll %d", &c.TimeoutPoll) == 1)
        {
            printf("TimeoutPoll: %d ms\n", c.TimeoutPoll);
            continue;
        }
        else if (sscanf(p1, " UserAgent %[^\\]", c.UserAgent) == 1)
        {
            printf("UserAgent: %s\n", c.UserAgent);
            continue;
        }
        else
        {
            printf("!!! Error read conf file: [%s]\n", p1);
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    if ((conf->MaxWindowSize/conf->MinWindowSize) < 2)
    {
        printf("!!! Error MaxWindowSize/MinWindowSize < 2\n");
        return -1;
    }

    if (conf->MaxConcurrentStreams <= 0)
    {
        c.MaxConcurrentStreams = 128;
    }

    printf("\n");

    return 0;
}
//======================================================================
int main(int count, char *strings[])
{
    char s[256];
    int run_ = 1, n;

    {
        c.ctx = InitCTX();
        SSL *ssl = SSL_new(c.ctx);
        printf("SSL version: %s\n", SSL_get_version(ssl));
        SSL_free(ssl);
        SSL_CTX_free(c.ctx);
    }
    
    signal(SIGPIPE, SIG_IGN);

    if (signal(SIGSEGV, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "<%s:%d> Error signal(SIGSEGV): %s\n", __func__, __LINE__, strerror(errno));
        return 1;
    }
    
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "<%s:%d> Error signal(SIGINT): %s\n", __func__, __LINE__, strerror(errno));
        return 1;
    }

    c.ctx = InitCTX();

    while (run_)
    {
        printf("============================================\n"
               "Input [Name request file] or [q: Exit]\n>>> ");
        
        fflush(stdin);
        fflush(stdout);
        std_in(s, sizeof(s));
        if (s[0] == 'q')
            break;

        printf("------------- config.txt --------------\n");
        if (read_conf_file())
        {
            continue;
        }

        string Path = conf->RequestsPath;
        Path += '/';
        Path += s;
        if ((n = read_req_file(Path.c_str())) < 0)
            continue;
        printf("-------------- %s ------------------\n", Path.c_str());
        printf("Server Port: ");
        fflush(stdout);
        std_in(c.port, sizeof(c.port));
        if (c.port[0] == 'q')
            break;
        if (c.port[0] == 'c')
            continue;
        if (is_number(c.port) == 0)
        {
            fprintf(stderr, "!!!   Error [Server Port: %s]\n", c.port);
            continue;
        }
        //--------------------------------------------------------------
        printf("Num Connections: ");
        fflush(stdout);
        std_in(s, sizeof(s));
        if (s[0] == 'q')
            break;
        if (s[0] == 'c')
            continue;
        if (sscanf(s, "%d", &c.num_connections) != 1)
        {
            fprintf(stderr, "!!!   Error [Num Connections: %s]\n", s);
            continue;
        }
        //--------------------------------------------------------------
        printf("Num Requests: ");
        fflush(stdout);
        std_in(s, sizeof(s));
        if (s[0] == 'q')
            break;
        if (s[0] == 'c')
            continue;
        if (sscanf(s, "%d", &c.num_req) != 1)
        {
            fprintf(stderr, "!!!   Error [Num Requests: %s]\n", s);
            continue;
        }
        //--------------------------------------------------------------
        time_t now;
        time(&now);
        printf("%s\n", ctime(&now));

        int servSocket = create_client_socket(Host, conf->port);
        if (servSocket < 0)
        {
            fprintf(stdout, "<%s:%d> Error: create_client_socket(%s:%s)\n", __func__, __LINE__, Host, c.port);
            continue;
        }

        if ((ai_family != AF_INET) && (ai_family != AF_INET6))
        {
            fprintf(stdout, "<%s:%d> Error: ai_family: %s\n", __func__, __LINE__, get_str_ai_family(ai_family));
            continue;
        }

        printf("IP: %s, FAMILY: %s\n", IP, get_str_ai_family(ai_family));
        snprintf(c.ip, sizeof(c.ip), "%s", IP);
        if (ai_family == AF_INET)
            c.create_sock = create_client_socket_ip4;
        else if (ai_family == AF_INET6)
            c.create_sock = create_client_socket_ip6;
        else
            exit(1);
        close(servSocket);
        
        struct rlimit lim;
        if (getrlimit(RLIMIT_NOFILE, &lim) == -1)
        {
            printf("<%s:%d> Error getrlimit(RLIMIT_NOFILE): %s\n", __func__, __LINE__, strerror(errno));
        }
        else
        {
            if ((conf->num_connections + 5) > (long)lim.rlim_cur)
            {
                if ((conf->num_connections + 5) <= (long)lim.rlim_max)
                {
                    lim.rlim_cur = conf->num_connections + 5;
                    setrlimit(RLIMIT_NOFILE, &lim);
                }
                else
                {
                    printf("<%s:%d> Error lim.rlim_max=%ld\n", __func__, __LINE__, (long)lim.rlim_max);
                    exit(1);
                }
            }
        }

        Path = conf->LogPath;
        Path += '/';
        Path += "error.log";
        int f_log = create_log_file(Path.c_str());
        if (f_log == -1)
            return 1;

        struct timeval time1;
        char s[256];
gettimeofday(&time1, NULL);
        int ret = create_connections();
get_time_connect(&time1, s, sizeof(s));
        time(&now);
        printf("  %s, requests: %d\n", s, ret);
        printf("%s\n", ctime(&now));
        close(f_log);
    }

    SSL_CTX_free(c.ctx);

    return 0;
}
