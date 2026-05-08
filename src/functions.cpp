#include "main.h"

using namespace std;
//======================================================================
void std_in(char *s, int len)
{
    char ch;
    
    while ((ch = getchar()) != '\n' && (len - 1))
    {
        *s++ = ch;
        len--;
    }
    *s = '\0';
    if (ch != '\n')
        while ((ch = getchar()) != '\n');
}
//======================================================================
int strcmp_case(const char *s1, const char *s2)
{
    char c1, c2;
    
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    for (; ; ++s1, ++s2)
    {
        c1 = *s1;
        c2 = *s2;
        if (!c1 && !c2) return 0;
        if (!c1) return -1;
        if (!c2) return 1;
        
        c1 += (c1 >= 'A') && (c1 <= 'Z') ? ('a' - 'A') : 0;
        c2 += (c2 >= 'A') && (c2 <= 'Z') ? ('a' - 'A') : 0;
        
        if (c1 > c2) return 1;
        if (c1 < c2) return -1;
    }

    return 0;
}
//======================================================================
const char *strstr_case(const char *s1, const char *s2)
{
    const char *p1, *p2;
    char c1, c2;
    
    if (!s1 || !s2) return NULL;
    if (*s2 == 0) return s1;

    for (; ; ++s1)
    {
        c1 = *s1;
        if (!c1) break;
        c2 = *s2;
        c1 += (c1 >= 'A') && (c1 <= 'Z') ? ('a' - 'A') : 0;
        c2 += (c2 >= 'A') && (c2 <= 'Z') ? ('a' - 'A') : 0;
        if (c1 == c2)
        {
            p1 = s1;
            p2 = s2;
            ++s1;
            ++p2;

            for (; ; ++s1, ++p2)
            {
                c2 = *p2;
                if (!c2) return p1;
                
                c1 = *s1;
                if (!c1) return NULL;

                c1 += (c1 >= 'A') && (c1 <= 'Z') ? ('a' - 'A') : 0;
                c2 += (c2 >= 'A') && (c2 <= 'Z') ? ('a' - 'A') : 0;
                if (c1 != c2)
                    break;
            }
        }
    }

    return NULL;
}
//======================================================================
int strlcmp_case(const char *s1, const char *s2, int len)
{
    char c1, c2;

    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    int diff = ('a' - 'A');

    for (; len > 0; --len, ++s1, ++s2)
    {
        c1 = *s1;
        c2 = *s2;
        if (!c1 && !c2) return 0;

        c1 += (c1 >= 'A') && (c1 <= 'Z') ? diff : 0;
        c2 += (c2 >= 'A') && (c2 <= 'Z') ? diff : 0;

        if (c1 != c2) return (c1 - c2);
    }

    return 0;
}
//======================================================================
const char *get_str_operation(OPERATION_TYPE n)
{
    switch (n)
    {
        case CONNECT:
            return "CONNECT";
        case SSL_CONNECT:
            return "SSL_CONNECT";
        case PREFACE_MESSAGE:
            return "PREFACE_MESSAGE";
        case SEND_SETTINGS:
            return "SEND_SETTINGS";
        case WORK_STREAM:
            return "WORK_STREAM";
    }

    return "?";
}
//======================================================================
const char *get_str_frame_type(FRAME_TYPE t)
{
    switch (t)
    {
        case DATA: // 0
            return "DATA";
        case HEADERS: // 1
            return "HEADERS";
        case PRIORITY: // 2
            return "PRIORITY";
        case RST_STREAM: // 3
            return "RST_STREAM";
        case SETTINGS: // 4
            return "SETTINGS";
        case PUSH_PROMISE: // 5
            return "PUSH_PROMISE";
        case PING: // 6
            return "PING";
        case GOAWAY: // 7
            return "GOAWAY";
        case WINDOW_UPDATE: // 8
            return "WINDOW_UPDATE";
        case CONTINUATION: // 9
            return "CONTINUATION";
        case ALTSVC: // 10 (0xA)
            return "ALTSVC";
        case ORIGIN: // 12 (0xC)
            return "ORIGIN";
        case CACHE_DIGEST: // 13 0x0D,
            return "CACHE_DIGEST";
        case PRIORITY_UPDATE: // 16 (0x10)
            return "PRIORITY_UPDATE";
    }

    switch ((int)t)
    {
        case 11:
            return "11 (0XB)";
        case 14:
            return "14 (0xE)";
        case 15:
            return "15 (0xF)";
    }

    return "?";
}
//======================================================================
const char *get_str_setting_param(int n)
{
    switch (n)
    {
        case 1:
            return "SETTINGS_HEADER_TABLE_SIZE";
        case 2:
            return "SETTINGS_ENABLE_PUSH";
        case 3:
            return "SETTINGS_MAX_CONCURRENT_STREAMS";
        case 4:
            return "SETTINGS_INITIAL_WINDOW_SIZE";
        case 5:
            return "SETTINGS_MAX_FRAME_SIZE";
        case 6:
            return "SETTINGS_MAX_HEADER_LIST_SIZE";
        case 7:
            return "SETTINGS_ACCEPT_CACHE_DIGEST";
        case 8:
            return "SETTINGS_ENABLE_CONNECT_PROTOCOL";
        case 9:
            return "SETTINGS_NO_RFC7540_PRIORITIES";
        default:
            return "?";
    }
    
    return "?";
}
//======================================================================
const char *get_http2_error(int err)
{
    switch (err)
    {
        case NO_ERROR: // 0
            return "NO_ERROR";
        case PROTOCOL_ERROR: // 1
            return "PROTOCOL_ERROR";
        case INTERNAL_ERROR: // 2
            return "INTERNAL_ERROR";
        case FLOW_CONTROL_ERROR: // 3
            return "FLOW_CONTROL_ERROR";
        case SETTINGS_TIMEOUT: // 4
            return "SETTINGS_TIMEOUT";
        case STREAM_CLOSED: // 5
            return "STREAM_CLOSED";
        case FRAME_SIZE_ERROR: // 6
            return "FRAME_SIZE_ERROR";
        case REFUSED_STREAM: // 7
            return "REFUSED_STREAM";
        case CANCEL: // 8
            return "CANCEL";
        case COMPRESSION_ERROR: // 9
            return "COMPRESSION_ERROR";
        case CONNECT_ERROR: // 10 (0xA)
            return "CONNECT_ERROR";
        case ENHANCE_YOUR_CALM: // 11 (0xB)
            return "ENHANCE_YOUR_CALM";
        case INADEQUATE_SECURITY: // 12 (0xC)
            return "INADEQUATE_SECURITY";
        case HTTP_1_1_REQUIRED: // 13 (0xD)
            return "HTTP_1_1_REQUIRED";
    }

    switch ((int)err)
    {
        case 14:
            return "14 (0XE)";
        case 15:
            return "15 (0xF)";
        case 16:
            return "16 (0x10)";
    }

    return "?";
}
//======================================================================
int read_line(FILE *f, char *s, int size)
{
    char *p = s;
    int ch, len = 0, wr = 1;

    while (((ch = getc(f)) != EOF) && (len < size))
    {
        if (ch == '\n')
        {
            *p = 0;
            if (wr == 0)
            {
                wr = 1;
                continue;
            }
            return len;
        }
        else if (wr == 0)
            continue;
        else if (ch == '#')
        {
            wr = 0;
        }
        else if (ch != '\r')
        {
            *(p++) = (char)ch;
            ++len;
        }
    }
    *p = 0;

    return len;
}
//======================================================================
int read_req_file_(FILE *f)
{
    char *p1, *p2, s[256];
    Method[0] = 0;
    Uri[0] = 0;
    Host[0] = 0;
    PostData[0] = 0;
    Range[0] = 0;

    while (fgets(s,sizeof(s), f))
    {
        if ((p1 = strpbrk(s, "\r\n")))
            *p1 = 0;
        p1 = s;
        while ((*p1 == ' ') || (*p1 == '\t'))
            p1++;

        if (*p1 == '#' || *p1 == 0)
            continue;
        else
        {
            if ((p2 = strchr(s, '#')))
                *p2 = 0;
        }
        
        if (sscanf(p1, " Method %15s", Method) == 1)
        {
            printf("Method: %s\n", Method);
            continue;
        }
        else if (sscanf(p1, " Path %1023s", Uri) == 1)
        {
            printf("Path: %s\n", Uri);
            continue;
        }
        else if (sscanf(p1, " Host %127s", Host) == 1)
        {
            printf("Host: %s\n", Host);
            continue;
        }
        /*else if (sscanf(p1, " UserAgent %511s", UserAgent) == 1)
        {
            printf("UserAgent: %s\n", UserAgent);
            continue;
        }*/
        else if (sscanf(p1, " PostData %511s", PostData) == 1)
        {
            printf("PostData: %s\n", PostData);
            continue;
        }
        else if (sscanf(p1, " Range %63s", Range) == 1)
        {
            printf("Range: %s\n", Range);
            continue;
        }
        else
        {
            printf("!!! Error read conf file: [%s]\n", p1);
            return -1;
        }
    }

    if (feof(f))
        return 0;
    return -1;
}
//======================================================================
int read_req_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
    {
        fprintf(stderr, " Error open request file(%s): %s\n", path, strerror(errno));
        return -1;
    }

    int n = read_req_file_(f);
    if (n < 0)
    {
        fprintf(stderr, "<%s> Error read_req_file()=%d\n", __func__, n);
        fclose(f);
        return -1;
    }

    fclose(f);
    return n;
}
//======================================================================
void set_size(ByteArray *ba, int size)
{
    int shift = 16;
    int i = 0;
    for ( ; shift >= 0; )
    {
        ba->set_byte(size>>shift, i);
        ++i;
        shift -= 8;
    }
}
//======================================================================
void set_id(ByteArray *ba, int d)
{
    int shift = 24;
    int i = 5;
    for ( ; shift >= 0; )
    {
        ba->set_byte(d>>shift, i);
        ++i;
        shift -= 8;
    }
}
//======================================================================
void set_bytes(ByteArray *ba, int data, int index)
{
    int shift = 24;
    for ( ; shift >= 0; )
    {
        ba->set_byte(data>>shift, index);
        ++index;
        shift -= 8;
    }
}
//======================================================================
int pow_(int x, int y)
{
    if (y < 0)
        return -1;
    int m = 1;
    for (int i = 0; i < y; ++i)
        m = m * x;
    return m;
}
//======================================================================
int int_to_bytes(ByteArray& buf, int data, int pref_len, int mask)
{
    int ret = 0;
    
    if (data < (pow_(2, pref_len) - 1))
    {
        buf.cat((data | mask));
        ++ret;
    }
    else
    {
        buf.cat((pow_(2, pref_len) - 1) | mask);
        ++ret;
        data = data - (pow_(2, pref_len) - 1);
        while (data > 128)
        {
            buf.cat(data % 128 + 128);
            ++ret;
            data = data / 128;
        }

        buf.cat((char)data);
        ++ret;
    }

    return ret;
}
//======================================================================
void set_frame_headers(Connect *c)
{
    c->headers.cpy("\0\0\0\1\0\0\0\0\0", 9);
    if (!strcmp(Method, "GET"))
        c->headers.set_byte(5, 4);
    else if (!strcmp(Method, "POST"))
        c->headers.set_byte(4, 4);
    int len = c->headers.size() - 9;
    set_size(&c->headers, len);
    set_id(&c->headers, c->stream_id);
}
//======================================================================
void add_header(Connect *conn, int ind)
{
    char s[8];
    s[0] = (ind | 0x80);
    conn->headers.cat(s, 1);
    int len = conn->headers.size() - 9;
    set_size(&conn->headers, len);
}
//======================================================================
/*void add_header(Connect *conn, int ind, const char *val)
{
    int len = (int)strlen(val);
    char s[8];
    s[0] = (ind | 0x40);
    s[1] = (char)len;
    conn->headers.cat(s, 2);
    conn->headers.cat(val, len);
    len = conn->headers.size() - 9;
    set_len(&conn->headers, len);
}*/
//======================================================================
void add_header(Connect *conn, int ind, int mask, const char *val, bool huffman)
{
    //if ((ind >= 8) && (ind <= 14))
    //    conn->respStatus = atoi(val);
    int len = (int)strlen(val);
    int prefix_len;
    switch (mask)
    {
        case 0x40:
            prefix_len = 6;
            break;
        case 0x10:
            prefix_len = 4;
            break;
        case 0x00:
            prefix_len = 4;
            break;
        default:
            prefix_len = 6;
            mask = 0x40;
    }

    int_to_bytes(conn->headers, ind, prefix_len, mask);

    if (huffman)
    {
        ByteArray buf;
        HuffmanCode huff;
        huff.encode(val, buf);
        int_to_bytes(conn->headers, buf.size(), 7, 0x80);
        conn->headers.cat(buf.ptr(), buf.size());
    }
    else
    {
        int_to_bytes(conn->headers, len, 7, 0);
        conn->headers.cat(val, len);
    }
    len = conn->headers.size() - 9;
    conn->headers.set_byte((len>>16) & 0xff, 0);
    conn->headers.set_byte((len>>8) & 0xff, 1);
    conn->headers.set_byte(len & 0xff, 2);
}
//======================================================================
void add_header(Connect *conn, int ind, const char *val)
{
    add_header(conn, ind, 0x00, val, true);
}
//======================================================================
void add_header(Connect *conn, const char *name, const char *val)
{
    int name_len = (int)strlen(name);
    int val_len = (int)strlen(val);
    char s[8];
    ByteArray ba;
    ba.cpy("\x00", 1);
    
    s[0] = (char)name_len;
    ba.cat(s, 1);
    ba.cat(name, name_len);
    
    s[0] = (char)val_len;
    ba.cat(s, 1);
    ba.cat(val, val_len);

    conn->headers.cat(ba.ptr(), ba.size());
    int len = conn->headers.size() - 9;
    set_size(&conn->headers, len);
}
//======================================================================
void set_frame_data(Connect *conn, const char *data, int len, int flag)
{
    conn->data.cpy("\0\0\0\0\0\0\0\0\0", 9);
    set_size(&conn->data, len);
    conn->data.set_byte(flag, 4);
    set_id(&conn->data, conn->stream_id);

    conn->data.cat(data, len);
}
//======================================================================
int set_frame_window_update(Stream *req, unsigned int size)
{
    if (req->frame_win_update.size())
    {
        fprintf(stderr, "[%lu/%d]<%s:%d> !!! req->frame_win_update.size()=%d/%u\n", req->num_conn, req->id, __func__, __LINE__, 
                        get_window_update_size(req->frame_win_update.ptr()), size);
        return 0;
    }

    if (size > 0x7fffffff)
    {
        fprintf(stderr, "[%lu/%d]<%s:%d> !!! size=0x%08X, req->frame_win_update.size()=%d\n", req->num_conn, req->id, __func__, __LINE__, 
                            size, get_window_update_size(req->frame_win_update.ptr()));
        return -1;
    }

    int id = req->id;
    req->serv_stream_windows_size += size;
    req->frame_win_update.cpy("\x00\x00\x04\x08\x00\x00\x00\x00\x00"  // 0-8
                               "\x00\x00\x00\x00", 13);               // 9-12
    set_bytes(&req->frame_win_update, id, 5);
    set_bytes(&req->frame_win_update, size, 9);
    return 0;
}
//======================================================================
int set_frame_window_update(Connect *con, unsigned int size) // 1 ... 2147483647
{
    if (con->frame_win_update.size())
    {
        fprintf(stderr, "[%lu]<%s:%d> !!! con->frame_win_update.size()=%d/%u\n", con->num_conn, __func__, __LINE__, 
                        get_window_update_size(con->frame_win_update.ptr()), size);
        return 0;
    }

    if (size > 0x7fffffff)
    {
        fprintf(stderr, "[%lu]<%s:%d> !!! size=0x%08X/%u\n", con->num_conn, __func__, __LINE__, size, size);
        return -1;
    }

    con->serv_connect_windows_size += size;
    con->frame_win_update.cpy("\x00\x00\x04\x08\x00\x00\x00\x00\x00"  // 0-8
                               "\x00\x00\x00\x00", 13);               // 9-12
    set_bytes(&con->frame_win_update, size, 9);
    return 0;
}
//======================================================================
int bytes_to_int(unsigned char prefix, int pref_len, const char *s, int size, int *len)
{
    int data = pow_(2, pref_len) - 1;
    if (prefix < data)
        data = prefix;
    else
    {
        unsigned char ch;
        for (int i = 0; (*len) < size; ++i)
        {
            ch = s[(*len)++];
            data = data + ((ch & 0x7f)<<(i*7));
            if (!(ch & 0x80))
                break;
        }
    }

    return data;
}
//======================================================================
int get_str(ByteArray *ba, std::string& str, int *offset)
{
    int ch;
    if (offset == NULL)
    {
        fprintf(stderr, "<%s:%d> Error: offset=NULL\n", __func__, __LINE__);
        return -1;
    }

    if ((ch = ba->get_byte((*offset)++)) < 0)
    {
        fprintf(stderr, "<%s:%d> Error get_byte()=%d\n", __func__, __LINE__, ch);
        return -1;
    }

    bool huffman = ch & 0x80;
    int val_len = ch & 0x7f;
    if (val_len == 0x7f)
    {
        val_len = bytes_to_int(ch & 0x7f, 7, ba->ptr(), ba->size(), offset);
        if (val_len <= 0)
        {
            fprintf(stderr, "<%s:%d> Error bytes_to_int()=%d\n", __func__, __LINE__, val_len);
            return -1;
        }
    }

    if ((val_len + *offset) > (int)ba->size())
    {
        fprintf(stderr, "<%s:%d> Error out of range [%d > %d]\n", __func__, __LINE__, val_len + *offset, ba->size());
        return -1;
    }

    if (huffman)
    {
        HuffmanCode huff;
        huff.decode(ba->ptr() + *offset, val_len, str);
    }
    else
        str.assign(ba->ptr() + *offset, val_len);
    (*offset) += val_len;
    return 0;
}
//======================================================================
int get_header(Connect *c, ByteArray *ba, int ind, std::string& name, std::string& val, int *offset)
{
    if (ind == 0x00)
    {
        if (get_str(ba, name, offset) < 0)
            return -1;
    }
    else
    {
        if (ind > 61)
        {
            if (c->dyn_tab)
            {
                Header *hd = c->dyn_tab->get(ind);
                if (hd)
                {
                    name = hd->name;
                    if (offset == NULL)
                    {
                        val = hd->val;
                        fprintf(stderr, "<<<< <<<< [%d] name & val from Dynamic Table:\n", ind);
                    }
                    else
                        fprintf(stderr, "<<<<      [%d] name from Dynamic Table:\n", ind);
                }
                else
                {
                    return -1;
                }
            }
            else
            {
                //name = "?";
                //if (offset == NULL)
                //    val = "?";
                fprintf(stderr, "<%s:%d> ind=%d Dynamic Table is not created\n", __func__, __LINE__, ind);
                return -1;
            }
        }
        else
        {
            name = static_tab[ind][0];
            if (offset == NULL)
                val = static_tab[ind][1];
        }
    }

    if (offset)
    {
        if (get_str(ba, val, offset) < 0)
            return -1;
    }

    return 0;
}
//======================================================================
int parse_headers(Connect *conn)
{
    int status = 0;
    int offset = 0;
    int ch;
    if (conn->frame_flags & 0x08) // PADDED (0x8)
        ++offset;
    if (conn->frame_flags & 0x20) // PRIORITY (0x20)
        offset += 5;
    std::string name;
    std::string val;

    for ( ; offset < conn->payload.size(); )
    {
        if ((ch = conn->payload.get_byte(offset++)) < 0)
        {
            fprintf(stderr, "<%s:%d> Error ch=%d, 0x%X\n", __func__, __LINE__, ch, ch);
            return -1;
        }

        if (ch >= 0x80) // [0x81 ... 0x8E, 0x90] from Static Table
        {               // [0xBC ... 0xFF]       from Dynamic Table
            int ind = bytes_to_int(ch & 0x7f, 7, conn->payload.ptr(), conn->payload.size(), &offset);
            if (get_header(conn, &conn->payload, ind, name, val, NULL) < 0)
                return -1;
        }
        else if ((ch >= 0x40) && (ch <= 0x7f)) // [0x40]<len><name><len><val>
        {                                      // [0x41 ... 0x7D]<index Static Table><len><val>
                                               // [0x7E ... 0x7F]<index Dyn Table><len><val>
            int ind = bytes_to_int(ch & 0x3f, 6, conn->payload.ptr(), conn->payload.size(), &offset);
            if (get_header(conn, &conn->payload, ind, name, val, &offset) < 0)
                return -1;
            if (conn->dyn_tab)
            {
                if (conn->dyn_tab->add(name, val) < 0)
                    return -1;
            }
        }
        else if ((ch >= 0x00) && (ch <= 0x0f)) // [0x00]<len><name><len><val>
        {                                      // [0x01 ... 0x0F]<index S/D><len><val>
            int ind = bytes_to_int(ch, 4, conn->payload.ptr(), conn->payload.size(), &offset);
            if (get_header(conn, &conn->payload, ind, name, val, &offset) < 0)
                return -1;
        }
        else if ((ch >= 0x10) && (ch <= 0x1f)) // [0x10]<len><name><len><val>
        {                                      // [0x11 ... 0x1F]<index S/D><len><val>
            int ind = bytes_to_int(ch & 0x0f, 4, conn->payload.ptr(), conn->payload.size(), &offset);
            if (get_header(conn, &conn->payload, ind, name, val, &offset) < 0)
                return -1;
        }
        else if ((ch >= 0x20) && (ch <= 0x3f)) // <0x20 ... 0x3F>
        {
            /*int size =*/ bytes_to_int(ch & 0x1f, 5, conn->payload.ptr(), conn->payload.size(), &offset);
            //fprintf(stderr, "<%s:%d> recv Dynamic Table Size Update: %d\n", __func__, __LINE__, size);
            continue;
        }
        else
        {
            fprintf(stderr, "<%s:%d> !!! 0x%02X\n", __func__, __LINE__, ch);
            return -1;
        }

        //fprintf(stderr, "[%lu] [0x%02X] [%s: %s]\n", conn->num_conn, ch, name.c_str(), val.c_str());
        if (name == "status")
        {
            if ((val != "200") && (val != "206"))
            {
                fprintf(stderr, "[%lu]<%s:%d> [%s: %s]\n", conn->num_conn, __func__, __LINE__, name.c_str(), val.c_str());
            }
            /*if (conn->print_all_headers)
            {
                status = atoi(val.c_str());
                conn->print_all_headers = false;
            }
            else*/
                return atoi(val.c_str());
        }
    }

    //if (conn->dyn_tab)
    //    conn->dyn_tab->print();
    
    return status;
}
//======================================================================
int parse_frame_headers(Connect *c)
{
    int offset = 9;
    int ch;
    char flags = c->headers.get_byte(4);
    if (flags & 0x08) // PADDED (0x8)
        ++offset;
    if (flags & 0x20) // PRIORITY (0x20)
        offset += 5;
    std::string name;
    name.reserve(32);
    std::string val;
    val.reserve(128);
    int size_ = offset;
fprintf(stderr, "\n[%lu]******** flags=0x%02X\n", c->num_conn, flags);
    for ( ; offset < ((int)c->headers.size() - size_); )
    {
        name = "?";
        val = "?";

        if ((ch = c->headers.get_byte(offset++)) < 0)
        {
            fprintf(stderr, "<%s:%d> Error ch=%d, 0x%X\n", __func__, __LINE__, ch, ch);
            return -1;
        }

        if (ch > 0x80)
        {// <0x81 ... 0xFF> ; static table: [0x81 ... 0x3D], dynamic table: [0x3E ...]
            int ind = bytes_to_int(ch & 0x7f, 7, c->headers.ptr(), c->headers.size(), &offset);
            if (get_header(c, &c->headers, ind, name, val, NULL) < 0)
                return -1;
        }
        else if ((ch >= 0x40) && (ch <= 0x7f))
        {// <0x40><len><name><len><val>, <0x41 ... 0x7F><index><len><val> ---> dyn_tab
            int ind = bytes_to_int(ch & 0x3f, 6, c->headers.ptr(), c->headers.size(), &offset);
            if (get_header(c, &c->headers, ind, name, val, &offset) < 0)
                return -1;
        }
        else if ((ch >= 0x00) && (ch <= 0x0f))
        {// <0x00><len><name><len><val>, <0x01 ... 0x0F><index><len><val>
            int ind = bytes_to_int(ch, 4, c->headers.ptr(), c->headers.size(), &offset);
            if (get_header(c, &c->headers, ind, name, val, &offset) < 0)
                return -1;
        }
        else if ((ch >= 0x10) && (ch <= 0x1f))
        {// <0x10><len><name><len><val>, <0x11 ... 0x1F><index><len><val>
            int ind = bytes_to_int(ch & 0x0f, 4, c->headers.ptr(), c->headers.size(), &offset);
            if (get_header(c, &c->headers, ind, name, val, &offset) < 0)
                return -1;
        }
        else if ((ch >= 0x20) && (ch <= 0x3f))
        {// Dynamic Table Size Update
            int size = bytes_to_int(ch & 0x1f, 5, c->headers.ptr(), c->headers.size(), &offset);
            fprintf(stderr, "[%lu] send Dynamic Table Size Update: %d\n", c->num_conn, size);
            continue;
        }
        else
        {
            fprintf(stderr, "<%s:%d> !!! 0x%02X\n", __func__, __LINE__, ch);
            return -1;
        }

        fprintf(stderr, "[%lu] [0x%02X] [%s: %s]\n", c->num_conn, ch, name.c_str(), val.c_str());
    }

    fprintf(stderr, "\n");

    return 0;
}
//======================================================================
int DynamicTable::add(std::string& name, std::string& val)
{
    int size = name.size() + val.size() + 32;
    if (max_table_size == 0)
        return 0;
    while ((table_size + size) > max_table_size)
    {
        --headers_num;
        delete [] table[headers_num].name;
        table[headers_num].name = NULL;
        table[headers_num].val = NULL;
        table_size -= table[headers_num].size;
        table[headers_num].size = 0;
        if (table_size <= 0)
        {
            return -1;
        }
    }

    if (headers_num >= max_headers_num)
    {
        fprintf(stderr, "<%s:%d> Error headers_num(%d) >= max_headers_num(%d)\n", __func__, __LINE__, 
                        headers_num, max_table_size);
        return -1;
    }

    for ( int i = headers_num; i > 0; --i)
    {
        table[i] = table[i - 1];
    }

    table[0].size = 0;

    table[0].name = new(std::nothrow) char [name.size() + val.size() + 2];
    if (!table[0].name)
    {
        table[0].val = NULL;
        return -1;
    }
    memcpy(table[0].name, name.c_str(), name.size() + 1);

    table[0].val = table[0].name + name.size() + 1;
    memcpy(table[0].val, val.c_str(), val.size() + 1);

    table[0].size = size;
    ++headers_num;
    table_size += size;

    return 0;
}
//======================================================================
int create_log_file(const char *file_name)
{
    int flog_err = open(file_name, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // O_APPEND O_TRUNC   
    if (flog_err == -1)
    {
        fprintf(stderr, "<%s:%d> Error create log_err(%s): %s\n", __func__, __LINE__, file_name, strerror(errno));
        exit(1);
    }

    dup2(flog_err, STDERR_FILENO);
    return flog_err;
}
//======================================================================
void hex_print_stderr(const char *s, int line, const void *p, int n)
{
    int count, addr = 0, col;
    unsigned char *buf = (unsigned char*)p;
    char str[18];
    fprintf(stderr, " [%s] <%d>--------------- HEX -----------------\n", s, line);
    for(count = 0; count < n;)
    {
        fprintf(stderr, "%08X  ", addr);
        for(col = 0, addr = addr + 0x10; (count < n) && (col < 16); count++, col++)
        {
            if (col == 8) fprintf(stderr, " ");
            fprintf(stderr, "%02X ", *(buf+count));
            str[col] = (*(buf + count) >= 32 && *(buf + count) < 127) ? *(buf + count) : '.';
        }
        str[col] = 0;
        if (col <= 8) fprintf(stderr, " ");
        fprintf(stderr, "%*s  %s\n",(16 - (col)) * 3, "", str);
    }

    //fprintf(stderr, "\n");
    fflush(stderr);
}
//======================================================================
const char *static_tab[][2] = {
     {"", ""},
     {"authority", ""},
     {"method", "GET"},
     {"method", "POST"},
     {"path", "/"},
     {"path", "/index.html"},
     {"scheme", "http"},
     {"scheme", "https"},
     {"status", "200"},
     {"status", "204"},
     {"status", "206"},
     {"status", "304"},
     {"status", "400"},
     {"status", "404"},
     {"status", "500"},
     {"accept-charset", ""},
     {"accept-encoding", "gzip, deflate"},
     {"accept-language", ""},
     {"accept-ranges", ""},
     {"accept", ""},
     {"access-control-allow-origin", ""},
     {"age", ""},
     {"allow", ""},
     {"authorization", ""},
     {"cache-control", ""},
     {"content-disposition", ""},
     {"content-encoding", ""},
     {"content-language", ""},
     {"content-length", ""},
     {"content-location", ""},
     {"content-range", ""},
     {"content-type", ""},
     {"cookie", ""},
     {"date", ""},
     {"etag", ""},
     {"expect", ""},
     {"expires", ""},
     {"from", ""},
     {"host", ""},
     {"if-match", ""},
     {"if-modified-since", ""},
     {"if-none-match", ""},
     {"if-range", ""},
     {"if-unmodified-since", ""},
     {"last-modified", ""},
     {"link", ""},
     {"location", ""},
     {"max-forwards", ""},
     {"proxy-authenticate", ""},
     {"proxy-authorization", ""},
     {"range", ""},
     {"referer", ""},
     {"refresh", ""},
     {"retry-after", ""},
     {"server", ""},
     {"set-cookie", ""},
     {"strict-transport-security", ""},
     {"transfer-encoding", ""},
     {"user-agent", ""},
     {"vary", ""},
     {"via", ""},
     {"www-authenticate", ""},
     {NULL, NULL}};
//======================================================================
void hex_print_stderr(const char *s1, const char *s2, int line, const void *p, int n)
{
    int count, addr = 0, col;
    unsigned char *buf = (unsigned char*)p;
    char str[18];
    fprintf(stderr, " [%s %s] <%d>--------------- HEX -----------------\n", s1, s2, line);
    for(count = 0; count < n;)
    {
        fprintf(stderr, "%08X  ", addr);
        for(col = 0, addr = addr + 0x10; (count < n) && (col < 16); count++, col++)
        {
            if (col == 8) fprintf(stderr, " ");
            fprintf(stderr, "%02X ", *(buf+count));
            str[col] = (*(buf + count) >= 32 && *(buf + count) < 127) ? *(buf + count) : '.';
        }
        str[col] = 0;
        if (col <= 8) fprintf(stderr, " ");
        fprintf(stderr, "%*s  %s\n",(16 - (col)) * 3, "", str);
    }

    //fprintf(stderr, "\n");
    fflush(stderr);
}
//======================================================================
void hex_print_stderr(const void *p, int n)
{
    int count, addr = 0, col;
    unsigned char *buf = (unsigned char*)p;
    char str[18];
    for(count = 0; count < n;)
    {
        fprintf(stderr, "%08X  ", addr);
        for(col = 0, addr = addr + 0x10; (count < n) && (col < 16); count++, col++)
        {
            if (col == 8) fprintf(stderr, " ");
            fprintf(stderr, "%02X ", *(buf+count));
            str[col] = (*(buf + count) >= 32 && *(buf + count) < 127) ? *(buf + count) : '.';
        }
        str[col] = 0;
        if (col <= 8) fprintf(stderr, " ");
        fprintf(stderr, "%*s  %s\n",(16 - (col)) * 3, "", str);
    }

    //fprintf(stderr, "\n");
    fflush(stderr);
}
