#ifndef _server_h
#define _server_h

#define ServerKEYFILE "mycert.pem"
#define ServerPASSWORD ""
#define DHFILE "dh1024.pem"
SSL_CTX* InitServerCTX(void);
void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile);
void Servlet(SSL* ssl);
int tcp_listen(void);
void load_dh_params(SSL_CTX *ctx,char *file);
void generate_eph_rsa_key(SSL_CTX *ctx);

#endif

