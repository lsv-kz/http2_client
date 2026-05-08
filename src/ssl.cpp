#include "main.h"

using namespace std;
//======================================================================
SSL_CTX* InitCTX()
{
    SSL_library_init();
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    //method = TLSv1_2_client_method();
    method = TLS_client_method();
    ctx = SSL_CTX_new(method);
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    if (ctx_set_alpn_protos(ctx, alpn, sizeof(alpn)))
    {
        fprintf(stderr, "<%s:%d> Error ctx_set_alpn_protos()\n", __func__, __LINE__);
        exit(1);
    }

    return ctx;
}
//======================================================================
int ctx_set_alpn_protos(SSL_CTX *ctx, const unsigned char *alpn, unsigned int size)
{
    if (SSL_CTX_set_alpn_protos(ctx, alpn, size))
    {
        fprintf(stderr, "Error SSL_CTX_set_alpn_protos\n");
        return -1;
    }
    return 0;
}
//======================================================================
const char *ssl_strerror(int err)
{
    switch (err)
    {
        case SSL_ERROR_NONE:
            return "SSL_ERROR_NONE";
        case SSL_ERROR_SSL:
            return "SSL_ERROR_SSL";
        case SSL_ERROR_WANT_READ:
            return "SSL_ERROR_WANT_READ";
        case SSL_ERROR_WANT_WRITE:
            return "SSL_ERROR_WANT_WRITE";
        case SSL_ERROR_WANT_X509_LOOKUP:
            return "SSL_ERROR_WANT_X509_LOOKUP";
        case SSL_ERROR_SYSCALL:
            //fprintf(stderr, "SSL_ERROR_SYSCALL(%s)\n", strerror(errno));
            return "SSL_ERROR_SYSCALL";
        case SSL_ERROR_ZERO_RETURN:
            return "SSL_ERROR_ZERO_RETURN";
        case SSL_ERROR_WANT_CONNECT:
            return "SSL_ERROR_WANT_CONNECT";
        case SSL_ERROR_WANT_ACCEPT:
            return "SSL_ERROR_WANT_ACCEPT";
    }
    
    return "?";
}
//======================================================================
int ssl_read(Connect *con, char *buf, int len)
{
    ERR_clear_error();
    int ret = SSL_read(con->ssl, buf, len);
    if (ret <= 0)
    {
        con->ssl_err = SSL_get_error(con->ssl, ret);
        if (con->ssl_err == SSL_ERROR_ZERO_RETURN)
        {
            return 0;
        }
        else if (con->ssl_err == SSL_ERROR_WANT_READ)
        {
            con->ssl_err = 0;
            return ERR_TRY_AGAIN;
        }
        else if (con->ssl_err == SSL_ERROR_WANT_WRITE)
        {
            fprintf(stderr, "<%s:%d> ??? Error SSL_read(): SSL_ERROR_WANT_WRITE\n", __func__, __LINE__);
            con->ssl_err = 0;
            return ERR_TRY_AGAIN;
        }
        else
        {
            fprintf(stderr, "<%s:%d> Error SSL_read()=%d: %s\n", __func__, __LINE__, ret, ssl_strerror(con->ssl_err));
            return -1;
        }
    }
    else
    {
        return ret;
    }
}
//======================================================================
int ssl_write(Connect *con, const char *buf, int len)
{
    ERR_clear_error();
    if (len <= 0)
    {
        fprintf(stderr, "<%s:%d> Error len=%d\n", __func__, __LINE__, len);
        return -1;
    }
    int ret = SSL_write(con->ssl, buf, len);
    if (ret <= 0)
    {
        con->ssl_err = SSL_get_error(con->ssl, ret);
        if (con->ssl_err == SSL_ERROR_WANT_WRITE)
        {
            con->ssl_err = 0;
            return ERR_TRY_AGAIN;
        }
        else if (con->ssl_err == SSL_ERROR_WANT_READ)
        {
            fprintf(stderr, "<%s:%d> ??? Error SSL_write(): %s, op=%s\n", __func__, __LINE__, ssl_strerror(con->ssl_err), get_str_operation(con->operation));
            con->ssl_err = 0;
            return ERR_TRY_AGAIN;
        }
        fprintf(stderr, "<%s:%d> Error SSL_write()=%d: %s, errno=%d\n", __func__, __LINE__, 
                    ret, ssl_strerror(con->ssl_err), errno);
        return -1;
    }
    else
        return ret;
}
