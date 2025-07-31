// Grupo SD-01
// Miguel Angel - fc65675
// Gonçalo Moreira - fc56571
// Liliana Valente - fc59846

#include <unistd.h>
#include "message-private.h"

int write_all(int sock, void *buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = write(sock, buf + sent, len - sent);
        if (n < 0) return -1;
        sent += n;
    }
    return sent;
}

int read_all(int sock, void *buf, int len) {
    int received = 0;
    while (received < len) {
        int n = read(sock, buf + received, len - received);
        if (n < 0) return -1;
        received += n;
    }
    return received;
}
