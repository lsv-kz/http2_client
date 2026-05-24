#include "main.h"

using namespace std;
//======================================================================
static Connect *start_list, *end_list;
static Connect **conn_array;

static struct pollfd *poll_fd;
static int num_poll;

static long long recv_all_bytes = 0;

int true_connect;

const char preface_message[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

int recv_frame(Connect *con);

int max_work_streams;
//======================================================================
void del_from_list(Connect *conn)
{
    if (conn->prev && conn->next)
    {
        conn->prev->next = conn->next;
        conn->next->prev = conn->prev;
    }
    else if (conn->prev && !conn->next)
    {
        conn->prev->next = conn->next;
        end_list = conn->prev;
    }
    else if (!conn->prev && conn->next)
    {
        conn->next->prev = conn->prev;
        start_list = conn->next;
    }
    else if (!conn->prev && !conn->next)
        start_list = end_list = NULL;
}
//======================================================================
void close_connect(Connect *conn)
{
    del_from_list(conn);
    if ((conn->ssl_err != SSL_ERROR_SSL) && (conn->ssl_err != SSL_ERROR_SYSCALL))
        SSL_shutdown(conn->ssl);
    SSL_free(conn->ssl);
    shutdown(conn->servSocket, SHUT_RDWR);
    close(conn->servSocket);
    delete conn;
}
//======================================================================
int http2_connection(Connect *con)
{
    if (con->operation == CONNECT)
    {
        con->operation = SSL_CONNECT;
        con->events = POLLIN | POLLOUT;
        con->sock_timer = 0;
    }
    else if (con->operation == SSL_CONNECT)
    {
        int ret = SSL_connect(con->ssl);
        if (ret < 1)
        {
            con->ssl_err = SSL_get_error(con->ssl, ret);
            if (con->ssl_err == SSL_ERROR_WANT_READ)
            {
                con->events = POLLIN;
                //fprintf(stderr, "[%lu]<%s:%d> SSL_ERROR_WANT_READ\n", con->num_conn, __func__, __LINE__);
            }
            else if (con->ssl_err == SSL_ERROR_WANT_WRITE)
            {
                con->events = POLLOUT;
                fprintf(stderr, "[%lu]<%s:%d> SSL_ERROR_WANT_WRITE\n", con->num_conn, __func__, __LINE__);
            }
            else
            {
                fprintf(stderr, "[%lu]<%s:%d> SSL_connect()=%d: %s\n", con->num_conn, __func__, __LINE__, ret, ssl_strerror(con->ssl_err));
                close_connect(con);
                return -1;
            }
        }
        else
        {
            const unsigned char *data = NULL;
            unsigned int alpnlen = 0;
            SSL_get0_alpn_selected(con->ssl, &data, &alpnlen);
            if (data)
            {
                if (memcmp(data, &alpn[1], alpnlen))
                {
                    fprintf(stderr, "<%s:%d> Error: protocol is not http2\n", __func__, __LINE__);
                    hex_print_stderr(__func__, __LINE__, data, alpnlen);
                    exit(1);
                }
            }
            else
            {
                fprintf(stderr, "[%lu]<%s:%d> SSL_get0_alpn_selected failed, alpnlen=%u\n", con->num_conn, __func__, __LINE__, alpnlen);
                close_connect(con);
                return -1;
            }

            con->operation = PREFACE_MESSAGE;
            con->events = POLLOUT;
            con->sock_timer = 0;
        }
    }
    else if (con->operation == PREFACE_MESSAGE)
    {
        if (SSL_write(con->ssl, preface_message, strlen(preface_message)) == 24)
        {
            con->operation = SEND_SETTINGS;
            con->events = POLLOUT;
            con->sock_timer = 0;
            set_frame_window_update(con, conf->MaxWindowSize);
        }
        else
        {
            fprintf(stderr, "[%lu]<%s:%d> Error send PREFACE_MESSAGE\n", con->num_conn, __func__, __LINE__);
            close_connect(con);
            return -1;
        }
    }
    else if (con->operation == SEND_SETTINGS)
    {
        if ((con->revents & POLLOUT) && (con->settings.size()))
        {
            int ret = write_to_client(con, con->settings.ptr(), con->settings.size());
            if (ret <= 0)
            {
                fprintf(stderr, "<%s:%d> Error send frame SETTINGS\n", __func__, __LINE__);
                if (ret != ERR_TRY_AGAIN)
                    close_connect(con);
                return ret;
            }

            con->sock_timer = 0;
            if (con->settings.get_byte(4) == 1)
                con->send_settings_ack = true;
            con->settings.init();
        }

        if (con->revents & POLLIN)
        {
            int ret = recv_frame(con);
            if (ret < 0)
            {
                if (ret == -1)
                    close_connect(con);
                return ret;
            }
        }

        if (con->recv_settings_ack && con->send_settings_ack)
        {
            con->operation = WORK_STREAM;
            con->send_headers = true;

            create_frame_headers(con);
            con->req_array = new(std::nothrow) Stream* [con->max_req];

            for (int i = 0; i < con->max_req; ++i)
            {
                Stream *req = con->new_stream(i * 2 + 1);
                if (!req)
                {
                    fprintf(stderr, "[%lu]<%s:%d> Error create Stream, id=%d\n", con->num_conn, 
                        __func__, __LINE__, i * 2 + 1);
                    exit(1);
                }

                con->req_array[i] = req;
            }
        }
    }

    return 0;
}
//======================================================================
int recv_frame(Connect *con)
{
    if (con->len_frame_head < 9)
    {
        int ret = read_from_client(con, con->frame_head + con->len_frame_head, sizeof(con->frame_head) - con->len_frame_head);
        if (ret < 0)
        {
            if (ret != ERR_TRY_AGAIN)
            {
                fprintf(stderr, "[%lu]<%s:%d> Error read_from_client()=%d\n", con->num_conn, __func__, __LINE__, ret);
            }
            return ret;
        }
        else if (ret == 0)
        {
            fprintf(stderr, "[%lu]<%s:%d> Error read_from_client()=%d\n", con->num_conn, __func__, __LINE__, ret);
            return -1;
        }
        else
        {
            con->len_frame_head += ret;
            if (con->len_frame_head < 9)
            {
                return 0;
            }

            con->sock_timer = 0;
            con->frame_size = ((unsigned char)con->frame_head[0]<<16) + 
                    ((unsigned char)con->frame_head[1]<<8) + (unsigned char)con->frame_head[2];
            con->frame_type = (FRAME_TYPE)con->frame_head[3];
            con->frame_flags = con->frame_head[4];
            con->frame_id = (((unsigned char)con->frame_head[5] & 0x7f)<<16) + ((unsigned char)con->frame_head[6]<<16) + 
                ((unsigned char)con->frame_head[7]<<8) + (unsigned char)con->frame_head[8];
        }
    }

    if (con->frame_size > 0)
    {
        char buf[16384 + 9];
        int rd = sizeof(buf);
        if (con->frame_size < rd)
            rd = con->frame_size;
        int ret = read_from_client(con, buf, rd);
        if (ret == ERR_TRY_AGAIN)
        {
            //fprintf(stderr, "<%s:%d> Error ERR_TRY_AGAIN\n", __func__, __LINE__);
            return ERR_TRY_AGAIN;
        }
        else if (ret <= 0)
        {
            fprintf(stderr, "[%lu]<%s:%d> Error read_from_client()=%d(%d)\n", con->num_conn, __func__, __LINE__, ret, con->frame_size);
            return -1;
        }

        con->payload.cat(buf, ret);

        con->frame_size -= ret;
        con->sock_timer = 0;

        if (con->frame_type == DATA)
        {
            con->read_bytes += ret;
            recv_all_bytes += ret;
            con->serv_connect_window_size -= ret;

            Stream *req = con->req_array[con->frame_id/2];
            if (req)
            {
                req->recv_bytes += ret;
                req->serv_stream_window_size -= ret;
            }
            else
            {
                fprintf(stderr, "[%lu]<%s:%d> Error Stream %d not found\n", con->num_conn, __func__, __LINE__, con->frame_id);
                return -1;
            }
        }

        if (con->frame_size > 0)
        {
            return 0;
        }

        con->len_frame_head = 0;

        if (con->frame_type == DATA)
        {
            if (con->frame_flags & FLAG_PADDED)
            {
                fprintf(stderr, "[%lu]<%s:%d> DATA %d, flags=0x%02X, id=%d\n", con->num_conn, 
                            __func__, __LINE__, ret, con->frame_flags, con->frame_id);
            }

            if (con->print_entity)
            {
                fwrite(con->payload.ptr(), 1, con->payload.size(), stderr);
                fprintf(stderr, "\n");
                if (con->frame_flags & FLAG_END_STREAM)
                {
                    fprintf(stderr, "[%lu]* EXIT, flags=0x%02X, id=%d\n", con->num_conn, con->frame_flags, con->frame_id);
                    exit(0);
                }
                fprintf(stderr, "[%lu]** EXIT, flags=0x%02X, id=%d\n", con->num_conn, con->frame_flags, con->frame_id);
                fflush(stderr);
                exit(0);
            }

            Stream *r = con->req_array[con->frame_id/2];
            if (r)
            {
                if (con->frame_flags & FLAG_END_STREAM)
                {
                    --con->num_work_stream;
                    ++true_connect;
                    ++con->num_close_stream;

                    delete r;
                    con->req_array[con->frame_id/2] = NULL;

                    if (con->num_close_stream >= con->max_req)
                    {
                        con->send_goaway = true;
                    }
                }
            }
            else
            {
                fprintf(stderr, "[%lu]<%s:%d> Error: Stream *r=NULL, id=%d\n", con->num_conn, __func__, __LINE__, con->frame_id);
                return -1;
            }
        }
        else if (con->frame_type == HEADERS)
        {
            if (con->frame_flags & FLAG_PADDED)
            {
                fprintf(stderr, "[%lu]<%s:%d> HEADERS flags=0x%02X, id=%d\n", con->num_conn, 
                        __func__, __LINE__, con->frame_flags, con->frame_id);
            }

            int status = parse_headers(con);
            if ((status != 200) && (status != 206))
            {
                con->print_entity = true;
                //con->send_goaway = true;
                con->payload.init();
                if (con->frame_flags & FLAG_END_STREAM)
                {
                    exit(0);
                }
                return 0;
            }

            if (con->frame_flags & FLAG_END_STREAM)
            {
                --con->num_work_stream;
                ++true_connect;
                ++con->num_close_stream;
                if (con->num_close_stream >= con->max_req)
                    con->send_goaway = true;
            }
        }
        else if (con->frame_type == SETTINGS)
        {
            if (con->operation != SEND_SETTINGS)
            {
                fprintf(stderr, "<%s:%d> recv SETTINGS, Error: operation != SEND_SETTINGS\n", __func__, __LINE__);
                con->send_goaway = true;
            }
            else
            {
                if (con->send_settings_ack == false)
                {
                    con->settings.cpy("\x00\x00\x00\x04\x01\x00\x00\x00\x00", 9);
                    for (int i = 0; i < (con->payload.size()/6); ++i)
                    {
                        int ind = i * 6;
                        if (con->payload.get_byte(ind + 1) == 3)
                        {// SETTINGS_MAX_CONCURRENT_STREAMS
                            long n = (unsigned char)con->payload.get_byte(ind + 5);
                            n += ((unsigned char)con->payload.get_byte(ind + 4)<<8);
                            n += ((unsigned char)con->payload.get_byte(ind + 3)<<16);
                            n += ((unsigned char)con->payload.get_byte(ind + 2)<<24);
                            if (n < conf->MaxConcurrentStreams)
                                set_max_concurrent_streams(n);
                        }
                        else if (con->payload.get_byte(ind + 1) == 4)
                        {// SETTINGS_INITIAL_WINDOW_SIZE
                            long n = (unsigned char)con->payload.get_byte(ind + 5);
                            n += ((unsigned char)con->payload.get_byte(ind + 4)<<8);
                            n += ((unsigned char)con->payload.get_byte(ind + 3)<<16);
                            n += ((unsigned char)con->payload.get_byte(ind + 2)<<24);
                        }
                    }
                }
            }
        }
        else if (con->frame_type == WINDOW_UPDATE)
        {
            
        }
        else if (con->frame_type == GOAWAY)
        {
            fprintf(stderr, "[%lu]  recv GOAWAY [%s], %s, %d, %d, %d\n", con->num_conn, get_http2_error(con->payload.get_byte(7)), 
                            get_str_operation(con->operation), con->num_close_stream, con->send_headers, con->index_req);
            return -1;
        }
        else if (con->frame_type == RST_STREAM)
        {
            fprintf(stderr, "[%lu/%d] recv RST_STREAM err: %d\n", con->num_conn, con->frame_id, con->payload.get_byte(3));
            --con->num_work_stream;
            ++con->num_close_stream;
            if (con->num_close_stream >= con->max_req)
                con->send_goaway = true;
        }
        else
        {
            fprintf(stderr, "[%lu]<%s:%d> !!! frame_type=%d\n", con->num_conn, __func__, __LINE__, con->frame_type);
            con->send_goaway = true;
        }

        con->payload.init();
    }
    else
    {
        con->len_frame_head = 0;
        if (con->frame_type == DATA)
        {
            if (con->frame_flags & FLAG_END_STREAM)
            {
                if (con->print_entity)
                {
                    exit(0);
                }

                --con->num_work_stream;
                ++true_connect;
                ++con->num_close_stream;
                if (con->num_close_stream >= con->max_req)
                    con->send_goaway = true;
            }
            else
            {
                fprintf(stderr, "[%lu]<%s:%d> Error: frame DATA size=0 and frame_flags!=FLAG_END_STREAM, id=%d\n", con->num_conn, __func__, __LINE__, con->frame_id);
                return -1;
            }
        }
        else if (con->frame_type == SETTINGS)
        {
            con->recv_settings_ack = true;
            if (con->send_settings_ack == true)
                con->operation = WORK_STREAM;
        }
        else
        {
            fprintf(stderr, "[%lu]<%s:%d> !!! frame_type=%d\n", con->num_conn, __func__, __LINE__, con->frame_type);
            return -1;
        }
    }
    
    return 0;
}
//======================================================================
int send_frame(Connect *con, ByteArray *ba)
{
    if (ba->size())
    {
        int ret = write_to_client(con, ba->ptr(), ba->size());
        if (ret < 0)
        {
            fprintf(stderr, "<%s:%d> Error send frame DATA: ret=%d\n", __func__, __LINE__, ret);
            if (ret == ERR_TRY_AGAIN)
            {
                return ERR_TRY_AGAIN;
            }

            return -1;
        }

        con->sock_timer = 0;
        ba->init();
        return ret;
    }
    else
    {
        return -1;
    }
}
//======================================================================
void create_frame_headers(Connect *c)
{
    char s[1024];
    set_frame_headers(c);
    if (!strcmp(Method, "GET"))
        add_header(c, 2);                                         // :method  GET
    else if (!strcmp(Method, "POST"))
    {
        add_header(c, 3);                                         // :method  POST
        add_header(c, 31, "application/x-www-form-urlencoded");   // content-type

        int len = strlen(PostData);
        snprintf(s, sizeof(s), "%d", len);
        add_header(c, 28, s);                                     // content-length
    }

    add_header(c, 4, Uri);                                        // :path 

    snprintf(s, sizeof(s), "%5s:%s", Host, conf->port);
    add_header(c, 1, s);                                          // :authority

    add_header(c, 7);                                             // :scheme  https
    add_header(c, 58, conf->UserAgent);                           // user-agent

    if (!strcmp(Method, "POST"))
    {
        set_frame_data(c, PostData, strlen(PostData), 1);
        c->headers.cat(c->data.ptr(), c->data.size());
        c->data.init();
    }
    else
    {
        int len = (int)strlen(Range);
        if (len > 0)
        {
            add_header(c, 50, Range);
        }
    }
}
//======================================================================
int send_frame_headers(Connect *c)
{
    if ((c->num_work_stream >= (int)conf->MaxConcurrentStreams))
        return 0;

    for ( ; c->index_req < c->max_req; )
    {
        int id = c->index_req * 2 + 1;
        set_stream_id(&c->headers, id); // stream_id
        int ret = write_to_client(c, c->headers.ptr(), c->headers.size());
        if (ret <= 0)
        {
            if (ret == ERR_TRY_AGAIN)
            {
                c->frame_try_again = &c->headers;
                return ERR_TRY_AGAIN;
            }

            fprintf(stderr, "[%lu/%d] Error send frame HEADERS\n", c->num_conn, id);
            return ret;
        }

        Stream *r = c->req_array[c->index_req];
        if (r)
        {
            set_frame_window_update(r, conf->MaxWindowSize);
            int ret = send_frame_window_update(c, r);
            if (ret < 0)
            {
                return ret;
            }
        }
        else
        {
            fprintf(stderr, "[%lu/%d] Error: stream[%d]=NULL\n", c->num_conn, id, c->index_req);
            return -1;
        }

        c->sock_timer = 0;
        ++c->num_work_stream;
        c->index_req++;
        if (c->index_req >= c->max_req)
        {
            c->send_headers = false;
            c->headers.init();
        }

        if (max_work_streams < c->num_work_stream)
            max_work_streams = c->num_work_stream;
    }

    c->index_req = 0;
    return 0;
}
//======================================================================
int get_window_update_size(const char *s)// 9 10 11 12
{
    int ret = 0;
    ret = ret | (unsigned char)s[9]<<24;
    ret = ret | (unsigned char)s[10]<<16;
    ret = ret | (unsigned char)s[11]<<8;
    ret = ret | (unsigned char)s[12];
    return ret;
}
//======================================================================
int send_frame_window_update(Connect *c)
{
    if (c->frame_win_update.size())
    {
        int ret = write_to_client(c, c->frame_win_update.ptr(), c->frame_win_update.size());
        if (ret < 0)
        {
            fprintf(stderr, "[%lu] Error send frame WINDOW_UPDATE: ret=%d\n", c->num_conn, ret);
            if (ret == ERR_TRY_AGAIN)
            {
                c->frame_try_again = &c->frame_win_update;
                return ERR_TRY_AGAIN;
            }

            return -1;
        }

        c->sock_timer = 0;
        c->frame_win_update.init();
        return ret;
    }
    else
    {
        fprintf(stderr, "[%lu] Error: conn->frame_win_update.size()=%d\n", c->num_conn, c->frame_win_update.size());
        return -1;
    }
}
//======================================================================
int send_frame_window_update(Connect *c, Stream *req)
{
    if (req->frame_win_update.size())
    {
        int ret = write_to_client(c, req->frame_win_update.ptr(), req->frame_win_update.size());
        if (ret < 0)
        {
            fprintf(stderr, "[%lu/%d] Error send frame WINDOW_UPDATE: ret=%d\n", c->num_conn, req->id, ret);
            if (ret == ERR_TRY_AGAIN)
            {
                c->frame_try_again = &req->frame_win_update;
                return ERR_TRY_AGAIN;
            }

            return -1;
        }

        c->sock_timer = 0;
        req->frame_win_update.init();
        return ret;
    }
    else
    {
        fprintf(stderr, "[%lu/%d] Error: stream->frame_win_update.size()=%d\n", c->num_conn, req->id, req->frame_win_update.size());
        return -1;
    }
}
//======================================================================
int set_window_update(Connect *c)
{
    for ( int i = 0; i < c->max_req; ++i)
    {
        Stream *r = c->req_array[i];
        if (r)
        {
            if (r->serv_stream_window_size < conf->MinWindowSize)
            {
                int ret = set_frame_window_update(r, conf->MaxWindowSize);
                if (ret < 0)
                {
                    fprintf(stderr, "[%lu]<%s:%d> !!! MaxWindowSize=%d, r->serv_stream_window_size=%d\n", c->num_conn, __func__, __LINE__, 
                                    conf->MaxWindowSize , r->serv_stream_window_size);
                }
            }
            if (r->frame_win_update.size())
            {
                int ret = send_frame_window_update(c, r);
                if (ret < 0)
                {
                    return ret;
                }
            }
        }
    }
    return 0;
}
//======================================================================
void worker(Connect *c)
{
    if (c->frame_try_again)
    {
        int ret = send_frame(c, c->frame_try_again);
        if (ret > 0)
        {
            unsigned int type = c->frame_try_again->get_byte(3);
            if (type == WINDOW_UPDATE)
                c->frame_try_again->init();
            else if (type == GOAWAY)
            {
                close_connect(c);
                return;
            }
            else if (type == HEADERS)
            {
        fprintf(stderr, "<%s:%d>!!! stream[%d]\n", __func__, __LINE__, c->index_req);
                Stream *r = c->req_array[c->index_req];
                if (r)
                {
                    set_frame_window_update(r, conf->MaxWindowSize);
                    int ret = send_frame_window_update(c, r);
                    if (ret < 0)
                    {
                        if (ret == -1)
                            close_connect(c);
                        return;
                    }
                    c->index_req++;
                }
                else
                {
                    fprintf(stderr, "<%s:%d> Error: stream[%d]=NULL\n", __func__, __LINE__, c->index_req);
                    close_connect(c);
                    return;
                }
            }
        }
        else if (ret == ERR_TRY_AGAIN)
            return;
        else if (ret < 0)
        {
            close_connect(c);
            return;
        }
    }

    if (c->send_goaway && (c->revents & POLLOUT))
    {
        if (c->data.size() == 0)
        {
            c->data.cpy("\x00\x00\x08\x07\x00\x00\x00\x00\x00"
                            "\x00\x00\x00\x00\x00\x00\x00\x00\x00", 17);
        }

        send_frame(c, &c->data);
        close_connect(c);
        return;
    }

    if ((c->frame_win_update.size()) && (c->revents & POLLOUT))
    {
        if (c->frame_win_update.size() == 0)
        {
            fprintf(stderr, "[%lu]<%s:%d> SEND_WINDOW_UPDATE frame_win_update.size()=%d\n", c->num_conn, 
                    __func__, __LINE__, c->frame_win_update.size());
            c->sock_timer = 0;
            return;
        }

        int ret = send_frame_window_update(c);
        if (ret < 0)
        {
            if (ret == -1)
                close_connect(c);
            return;
        }
    }

    if (c->revents & POLLIN)
    {
        if (recv_frame(c) == -1)
        {
            close_connect(c);
            return;
        }
    }

    if (c->send_headers && (c->revents & POLLOUT))
    {
        int ret = send_frame_headers(c);
        if (ret < 0)
        {
            if (ret == -1)
                close_connect(c);
            return;
        }
    }

    if (c->revents & POLLOUT)
    {
        if (set_window_update(c) == -1)
            close_connect(c);
    }
}
//======================================================================
void loop()
{
    recv_all_bytes = 0;
    while (1)
    {
        if (start_list == NULL)
        {
            fprintf(stdout, "<%s:%d> --- END recv from server %lld bytes [%d]---\n", __func__, __LINE__, recv_all_bytes, max_work_streams);
            break;
        }

        num_poll = 0;
        time_t t = time(NULL);
        Connect *con = start_list, *next = NULL;
        for ( int i = 0; con; con = next, i++)
        {
            next = con->next;

            if (con->sock_timer == 0)
                con->sock_timer = t;
            if ((t - con->sock_timer) >= conf->Timeout)
            {
                fprintf(stderr, "<%s:%d> Timeout=%lld, %s, pend=%d\n", __func__, __LINE__, 
                        (long long)t - con->sock_timer, get_str_operation(con->operation), SSL_pending(con->ssl));
                close_connect(con);
                continue;
            }

            poll_fd[i].fd = con->servSocket;
            if (con->operation == PREFACE_MESSAGE)
            {
                poll_fd[i].events = POLLOUT;
                num_poll++;
            }
            else if (con->operation == CONNECT)
            {
                poll_fd[i].events = con->events;
                num_poll++;
            }
            else if (con->operation == SSL_CONNECT)
            {
                poll_fd[i].events = con->events;
                num_poll++;
            }
            else if (con->operation == SEND_SETTINGS)
            {
                poll_fd[i].events = 0;
                if (con->recv_settings_ack == false)
                {
                    if (SSL_pending(con->ssl))
                    {
                        con->revents = POLLIN;
                        http2_connection(con);
                    }
                    else
                        poll_fd[i].events |= POLLIN;
                }
                
                if (con->settings.size())
                    poll_fd[i].events |= POLLOUT;

                num_poll++;
            }
            else if (con->operation == WORK_STREAM)
            {
                poll_fd[i].events = 0;
                if (con->send_goaway)
                {
                    poll_fd[i].events |= POLLOUT;
                }
                else
                {
                    if (con->send_headers && (con->num_work_stream < conf->MaxConcurrentStreams))
                    {
                        poll_fd[i].events |= POLLOUT;
                    }

                    if ((con->serv_connect_window_size < conf->MinWindowSize) || con->frame_win_update.size())
                    {
                        if (con->frame_win_update.size() == 0)
                        {
                            set_frame_window_update(con, conf->MaxWindowSize);
                        }
                        poll_fd[i].events = POLLOUT;
                    }

                    for (int j = 0; (j < con->max_req) && (poll_fd[i].events != POLLOUT); ++j)
                    {
                        Stream *r = con->req_array[j];
                        if (r)
                        {
                            if (r->serv_stream_window_size < conf->MinWindowSize)
                            {
                                poll_fd[i].events = POLLOUT;
                            }
                        }
                    }

                    int ret = 0, pending = 0;
                    while ((pending = SSL_pending(con->ssl)) > 0)
                    {
                        if ((ret = recv_frame(con)) < 0)
                        {
                            if (ret == -1)
                                close_connect(con);
                            break;
                        }
                    }

                    if (ret == -1)
                        continue;
                    else
                        poll_fd[i].events |= POLLIN;
                }

                if (poll_fd[i].events)
                    num_poll++;
            }
        }

        int ret_poll = poll(poll_fd, num_poll, conf->TimeoutPoll);
        if (ret_poll == -1)
        {
            fprintf(stderr, "<%s:%d> Error poll(, %d, ): %s\n",  __func__, __LINE__, num_poll, strerror(errno));
            
            for (int i = 0; i < num_poll; ++i)
            {
                fprintf(stderr, "<%d:%d> fd=%d, 0x%02X\n",  i, __LINE__, poll_fd[i].fd, poll_fd[i].events);
            }
            
            return;
        }
        else if (ret_poll == 0)
        {
            continue;
        }

        con = start_list, next = NULL;
        for ( int i = 0; con; con = next, i++)
        {
            next = con->next;
            con->revents = poll_fd[i].revents;
            if (poll_fd[i].revents & (POLLIN | POLLOUT))
            {
                if ((con->operation == SSL_CONNECT) ||
                    (con->operation == PREFACE_MESSAGE) ||
                    (con->operation == SEND_SETTINGS)
                     )
                {
                    http2_connection(con);
                }
                else
                {
                    worker(con);
                }
            }
        }
    }
}
//======================================================================
void delete_connect()
{
    if (poll_fd)
        delete [] poll_fd;
    Connect *start = start_list, *next = NULL;
    for ( ; start; start = next)
    {
        next = start->next;
        close(start->servSocket);
        delete start;
        //start = NULL;
    }

    start_list = NULL;
}
//======================================================================
Connect *create_connect()
{
    Connect *conn = new(nothrow) Connect;
    if (!conn)
        fprintf(stderr, "<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
    return conn;
}
//======================================================================
int create_connections()
{
    start_list = end_list = NULL;
    poll_fd = NULL;
    true_connect = 0;
    max_work_streams = 0;
fprintf(stdout, "<%s:%d> ---------num_connections=%d--------\n", __func__, __LINE__, conf->num_connections);
    Connect *con;

    poll_fd = new(nothrow) struct pollfd [conf->num_connections];
    if (!poll_fd)
    {
        fprintf(stderr,"<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
        exit(1);
    }

    int i = 0;
    for ( ; i < conf->num_connections; ++i)
    {
        con = create_connect();
        if (!con)
        {
            delete_connect();
            return 0;
        }

        con->num_conn = i;
        con->max_req = conf->num_req;
        int err;
        con->servSocket = conf->create_sock(conf->ip, conf->port, &err);
        if (con->servSocket < 0)
        {
            fprintf(stderr, "[%d]<%s:%d> Error create_sock()\n", i, __func__, __LINE__);
            delete_connect();
            return 0;
        }

        con->ssl = SSL_new(conf->ctx);
        if (!con->ssl)
        {
            fprintf(stderr, "[%d]<%s:%d> Error SSL_new()\n", i, __func__, __LINE__);
            delete_connect();
            return 0;
        }

        if (SSL_set_tlsext_host_name(con->ssl, Host) != 1)
        {
            printf("[%d]<%s:%d> Error SSL_set_tlsext_host_name(%s)\n", i, __func__, __LINE__, Host);
            fprintf(stderr, "[%d]<%s:%d> Error SSL_set_tlsext_host_name(%s)\n", i, __func__, __LINE__, Host);
            ERR_print_errors_fp(stderr);
        }

        if (SSL_set_alpn_protos(con->ssl, alpn, sizeof(alpn)))
        {
            fprintf(stderr, "Error SSL_CTX_set_alpn_protos\n");
            delete_connect();
            return 0;
        }

        if (!SSL_set_fd(con->ssl, con->servSocket))
        {
            fprintf(stderr, "[%d]<%s:%d> Error SSL_set_fd()\n", i, __func__, __LINE__);
            delete_connect();
            return 0;
        }

        con->operation = SSL_CONNECT;
        con->events = POLLOUT | POLLIN;

        con->prev = end_list;
        if (start_list)
        {
            end_list->next = con;
            end_list = con;
        }
        else
            start_list = end_list = con;
    }

    conn_array = new(nothrow) Connect* [conf->num_connections];
    if (!conn_array)
    {
        fprintf(stderr, "<%s:%d> Error malloc(): %s\n", __func__, __LINE__, strerror(errno));
        exit(1);
    }

    loop();
    delete [] conn_array;
    delete_connect();
    return true_connect;
}
