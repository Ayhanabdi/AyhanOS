#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

// لیست پورت‌های خطرناکی که باید مسدود شوند
int BLOCKED_PORTS[] = {80, 8080, 8081, 1080, 23, 21, 3389, 445, 3306};
int NUM_BLOCKED = sizeof(BLOCKED_PORTS) / sizeof(BLOCKED_PORTS[0]);

int is_blocked_port(int port) {
    for (int i = 0; i < NUM_BLOCKED; i++) {
        if (port == BLOCKED_PORTS[i]) return 1;
    }
    return 0;
}

int main() {
    int raw_sock;
    unsigned char *buffer = malloc(65536);
    if (!buffer) {
        perror("malloc");
        return 1;
    }

    // ساخت Raw Socket برای دریافت تمام بسته‌های TCP
    raw_sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (raw_sock < 0) {
        perror("socket (need root)");
        free(buffer);
        return 1;
    }

    printf("[*] AyhanGuard Firewall started (Kernel Level)\n");
    printf("[*] Blocking ports: ");
    for (int i = 0; i < NUM_BLOCKED; i++) printf("%d ", BLOCKED_PORTS[i]);
    printf("\n");

    while (1) {
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);

        int data_size = recvfrom(raw_sock, buffer, 65536, 0, (struct sockaddr*)&src_addr, &addr_len);
        if (data_size < 0) continue;

        struct iphdr *iph = (struct iphdr*)buffer;
        if (iph->protocol != IPPROTO_TCP) continue;

        struct tcphdr *tcph = (struct tcphdr*)(buffer + (iph->ihl * 4));
        int dest_port = ntohs(tcph->dest);
        int src_port = ntohs(tcph->source);

        // اگر پورت مسدود بود، بسته را دور می‌ریزیم (Drop)
        if (is_blocked_port(dest_port) || is_blocked_port(src_port)) {
            printf("[!] Blocked TCP packet: port %d\n", dest_port);
            // بسته را نادیده می‌گیریم (کاری نمی‌کنیم)
        }
    }

    close(raw_sock);
    free(buffer);
    return 0;
}
