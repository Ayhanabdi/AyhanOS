#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>

// ساختار برای ذخیره وضعیت دانلود
struct progress_data {
    double last_runtime;
};

// تابع نمایش نوار پیشرفت
static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    struct progress_data *prog = (struct progress_data *)clientp;
    (void)ultotal;
    (void)ulnow;

    if (dltotal > 0) {
        double percentage = (double)dlnow / (double)dltotal * 100.0;
        double speed = 0;
        
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double now = ts.tv_sec + ts.tv_nsec / 1e9;
        if (prog->last_runtime > 0) {
            speed = (double)(dlnow) / (now - prog->last_runtime) / 1024.0 / 1024.0;
        }
        prog->last_runtime = now;

        printf("\r[");
        int pos = (int)(percentage / 2.5);
        for (int i = 0; i < 40; i++) {
            if (i < pos) printf("=");
            else if (i == pos) printf(">");
            else printf(" ");
        }
        printf("] %.1f%% (%.1f MB / %.1f MB) [%.2f MB/s]", 
               percentage, 
               (double)dlnow / (1024*1024), 
               (double)dltotal / (1024*1024),
               speed);
        fflush(stdout);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "AyhanDL - Secure Downloader for AyhanOS\n");
        fprintf(stderr, "Usage: %s <URL> <OutputFile> [Options]\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -c        Resume partial download\n");
        fprintf(stderr, "  -x <proxy> Use proxy (e.g., socks5://127.0.0.1:1080)\n");
        return 1;
    }

    const char *url = argv[1];
    const char *out_filename = argv[2];
    int resume = 0;
    const char *proxy = NULL;

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) resume = 1;
        else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) proxy = argv[++i];
    }

    // بررسی اینکه URL با HTTPS شروع می‌شود (اجبار به HTTPS)
    if (strncmp(url, "https://", 8) != 0) {
        fprintf(stderr, "[-] Error: AyhanOS requires HTTPS. HTTP is blocked.\n");
        return 1;
    }

    CURL *curl;
    FILE *fp;
    CURLcode res;
    struct progress_data prog = {0};

    // بررسی وجود فایل برای ادامه دانلود
    struct stat st;
    long resume_from = 0;
    if (resume && stat(out_filename, &st) == 0) {
        resume_from = st.st_size;
        fp = fopen(out_filename, "ab");
        printf("[*] Resuming from byte: %ld\n", resume_from);
    } else {
        fp = fopen(out_filename, "wb");
    }

    if (!fp) {
        perror("fopen");
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        // اجبار به TLS 1.2+ برای امنیت
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "AyhanDL/1.0 (AyhanOS)");
        
        // نوار پیشرفت
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        // ادامه دانلود
        if (resume_from > 0) {
            curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)resume_from);
        }

        // پروکسی
        if (proxy) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
        }

        printf("[*] AyhanDL: Downloading %s\n", url);
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            fprintf(stderr, "\n[-] Download failed: %s\n", curl_easy_strerror(res));
        } else {
            printf("\n[+] Download successful: %s\n", out_filename);
        }

        curl_easy_cleanup(curl);
    }
    
    fclose(fp);
    curl_global_cleanup();
    return 0;
}
