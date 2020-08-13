#include <stdio.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/dh.h>
#include <iostream>
#include "mongoose.h"
#include "base64.h"
#include <string.h>
#include "dns_sd.h"
#include <openssl/bn.h>
#include <vector>
#include "sha1.h"
#include "hmac.h"

static const char *s_http_port = "2138";
static struct mg_serve_http_opts s_http_server_opts;
static unsigned char DH_generator[1] = {2};

std::string urlDecode(std::string str)
{
    std::string ret;
    char ch;
    int i, ii, len = str.length();

    for (i = 0; i < len; i++)
    {
        if (str[i] != '%')
        {
            if (str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        }
        else
        {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}
static unsigned char DH_prime[] = {
    /* Well-known Group 1, 768-bit prime */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9,
    0x0f, 0xda, 0xa2, 0x21, 0x68, 0xc2, 0x34, 0xc4, 0xc6,
    0x62, 0x8b, 0x80, 0xdc, 0x1c, 0xd1, 0x29, 0x02, 0x4e,
    0x08, 0x8a, 0x67, 0xcc, 0x74, 0x02, 0x0b, 0xbe, 0xa6,
    0x3b, 0x13, 0x9b, 0x22, 0x51, 0x4a, 0x08, 0x79, 0x8e,
    0x34, 0x04, 0xdd, 0xef, 0x95, 0x19, 0xb3, 0xcd, 0x3a,
    0x43, 0x1b, 0x30, 0x2b, 0x0a, 0x6d, 0xf2, 0x5f, 0x14,
    0x37, 0x4f, 0xe1, 0x35, 0x6d, 0x6d, 0x51, 0xc2, 0x45,
    0xe4, 0x85, 0xb5, 0x76, 0x62, 0x5e, 0x7e, 0xc6, 0xf4,
    0x4c, 0x42, 0xe9, 0xa6, 0x3a, 0x36, 0x20, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

DH *dh;
static const char *response_fmt =
    "HTTP/1.0 200 OK\r\n"
    "Connection: close\r\n"
    "Content-Type: application/json\r\n"
    "\r\n"
    "{\"status\":101,\"statusString\":\"OK\",\"spotifyError\":0,\"version\":\"2.7.1\",\"libraryVersion\":\"1.2.2\",\"accountReq\":\"PREMIUM\",\"brandDisplayName\":\"librespot-org\",\"modelDisplayName\":\"cpot\",\"voiceSupport\":\"NO\",\"availability\":\"\",\"productID\":0,\"tokenType\":\"default\",\"groupStatus\":\"NONE\",\"resolverVersion\":\"0\",\"scope\":\"streaming,client-authorization-universal\",\"activeUser\":\"\",\"deviceID\":\"t8s2ogzatcbtgmkyrlqpagjkbc3e5d4ucbgnhlno\",\"remoteName\":\"DDD speaker\",\"publicKey\":\"%s\",\"deviceType\":\"COMPUTER\"}";
std::string ReplaceAll(std::string str, const std::string &from, const std::string &to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}
unsigned char my_priv_key[96];
unsigned char my_pub_key[96];

std::string findQueryParam(std::string query, std::string param)
{
    query = urlDecode(query);
    auto sub = query.substr(query.find(param + "="), query.length());
    return sub.substr(param.length() + 1, sub.find("&") - param.length() - 1);
}

static void ev_handler(struct mg_connection *nc, int ev, void *p)
{

    switch (ev)
    {

    case MG_EV_HTTP_REQUEST:
    {
        struct http_message *hm = (struct http_message *)p;
        std::string uri(hm->uri.p);
        //std::cout << uri << std::endl;

        //printf("%s\n", (char*)hm->uri);

        if (uri.find("getInfo") != std::string::npos)
        {
            auto encoded = base64_encode(my_pub_key, 96);

            //printf("%s\n", hm->uri.p);
            mg_printf(nc, response_fmt, encoded.c_str());
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }
        else if (uri.find("addUser") != std::string::npos)
        {
            size_t pos = uri.find("action=addUser");
            auto queryString = uri.substr(pos, uri.length());

            std::cout << findQueryParam(queryString, "userName") << std::endl;
        }

        //printf("%s\n", hm->uri.p);
        mg_printf(nc, response_fmt, "bruho");
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
    }
    }
}

unsigned char shared_key[96];
void ev_handler2(struct mg_connection *c, int ev, void *ev_data)
{
    struct mbuf *io = &c->recv_mbuf;
    switch (ev)
    {
    case MG_EV_CONNECT:
        if (c->flags & MG_F_CLOSE_IMMEDIATELY)
        {
            printf("%s\n", "Error connecting to server!");
            exit(EXIT_FAILURE);
        }
        else
        {
            std::string *data = new std::string("my location is woodward, usa");
            mg_send(c, data->c_str(), data->length());
            delete data;
            printf("%s\n", "Connected to server. Type a message and press enter.");
        }
        break;
    case MG_EV_RECV:
    {
        // std::string receivedString = io->buf;
        printf("%s\n", io->buf);
        std::string *data = new std::string("doopa");
        mg_send(c, data->c_str(), data->length());
        // std::cout << receivedString;
        // mg_send(nc, io->buf, io->len);  // Echo received data back
        mbuf_remove(io, io->len); // Discard data from recv buffer
    }
    break;
    // case MG_EV_POLL:
    //     mg_send(c, &(c->recv_mbuf), (int)c->recv_mbuf.len);
    //     mbuf_remove(&c->recv_mbuf, (int)c->recv_mbuf.len);
    default:
        break;
    }
}
int main()
{

    // dh = DH_new();
    // DH_set0_pqg(dh, BN_bin2bn(DH_prime, 96, NULL), NULL, BN_bin2bn(DH_generator, 1, NULL));
    // DH_generate_key(dh);
    // BN_bn2bin(DH_get0_priv_key(dh), my_priv_key);
    // BN_bn2bin(DH_get0_pub_key(dh), my_pub_key);

    // auto testQueryStr = "action=addUser&userName=fliperspotify&blob=wo8vzfJ7N1KTyNP0NmxqcG4od1IWNhAmECAwoGKjImOImrIhRTQtkRwYX1vCC5aHFqJoDLTT2ISHu8%2Fd7kddUXkW7f7Mk941GaFHchzANE%2FEFy18anPeHJSy9t97oZvpvNWAkfe6emhwaV1Q3I5IqAV36QeVZKOTLuAjXYsX1wbbG0BZvnXFpv4hVS3sCgn6tB2IKTA0eita06jxSkIh%2BbG%2BRlDpMDGQabRWvmr6F2pWueYRTWbei8C8zD7H3fhJXEt8volNd2YeJzJf0%2BTP1TXDTP%2BebhaWZ385KRjnC73JgBykNurUbMHjY42HoJv%2FDGGkWGdg7XW072ScyTLV8J1EE1KIhSHMpU1zKFmKImY%3D&clientKey=EsK2OvVmbubqRuhiK%2FvUiHEECR5bBka7nMI%2FnVPVNKKZX7FrU9za0VwM5IgvfLrf9b0RI6Fn3PT8Pe4F7NQTx5XwuvcEjiIIwbnFsifToh%2FKdfSFsGoCRXfbhzaUrJtd&tokenType=default&loginId=ea34a670c801fe0327051c38ea3c331d&deviceName=iMac+%28apultyna%29&deviceId=4e349539a56a7c60c7c60caede7a30c2c8b52fa9&version=2.7.1";
    // auto userName = findQueryParam(testQueryStr, "userName");
    // auto blob = findQueryParam(testQueryStr, "blob");
    // auto clientKey = findQueryParam(testQueryStr, "clientKey");
    // auto blobBytes = base64_decode(blob);
    // auto clientKeyBytes = base64_decode(clientKey);

    // size_t blobSize = blobBytes.size();
    // auto pubKey = BN_bin2bn(&clientKeyBytes[0], 96, NULL);
    // DH_compute_key(shared_key, pubKey, dh);

    // std::vector<unsigned char> iv(blobBytes.begin(), blobBytes.begin() + 16);
    // std::vector<unsigned char> encrypted(blobBytes.begin() + 16, blobBytes.end() - 20);
    // std::vector<unsigned char> checksum(blobBytes.end() - 20, blobBytes.end());
    // SHA1_CTX ctx;

    // unsigned char digest[20];
    // SHA1Init(&ctx);
    // SHA1Update(&ctx, shared_key, 96);
    // SHA1Final(digest, &ctx);

    // // sha1_hmac(digest, 16, "checksum", 8);

    // // std::cout << userName << std::endl
    // //           << blob << std::endl
    // //           << clientKey;

    // return 0;

    // const char *service = "_spotify-connect._tcp";
    // const char *name = "cspot";

    // DNSServiceRef ref = NULL;
    // TXTRecordRef txtRecord;
    // TXTRecordCreate(&txtRecord, 0, NULL);
    // TXTRecordSetValue(&txtRecord, "VERSION", 3, "1.0");
    // TXTRecordSetValue(&txtRecord, "CPath", 1, "/");
    // TXTRecordSetValue(&txtRecord, "Stack", 2, "SP");
    // DNSServiceRegister(&ref, 0, 0, name, service, NULL, NULL, htons(2707), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), NULL, NULL);
    // while(true);
    struct mg_mgr mgr;
    // struct mg_connection *nc;

    mg_mgr_init(&mgr, NULL);
    auto nc = mg_connect(&mgr, "localhost:5555", ev_handler2);
    if (nc != NULL)
    {
        for (;;)
        {
            mg_mgr_poll(&mgr, 1000);
        }
        return 1;
    }
    // printf("Starting web server on port %s\n", s_http_port);
    // nc = mg_bind(&mgr, s_http_port, ev_handler);
    // if (nc == NULL)
    // {
    //     printf("Failed to create listener\n");
    //     return 1;
    // }

    // // Set up HTTP server parameters
    // mg_set_protocol_http_websocket(nc);
    // s_http_server_opts.document_root = "."; // Serve current directory
    // s_http_server_opts.enable_directory_listing = "yes";

    for (;;)
    {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);
    return 0;
}