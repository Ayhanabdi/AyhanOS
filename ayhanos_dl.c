#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>

// ساختار برای ذخیره وضعیت دانلود
struct progress_data {
    double last_runtime;
    int quiet;
};

// تابع نمایش نوار پیشرفت
static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    struct progress_data *prog = (struct progress_data *)clientp;
    (void)ultotal;
    (void)ulnow;

    if (prog->quiet) return 0;

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

// تابع استخراج نام فایل از URL
void get_filename_from_url(const char *url, char *out_filename) {
    char *url_copy = strdup(url);
    char *last_slash = strrchr(url_copy, '/');
    if (last_slash && *(last_slash + 1) != '\0') {
        strcpy(out_filename, last_slash + 1);
    } else {
        strcpy(out_filename, "download");
    }
    free(url_copy);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "AyhanOSDL v3.0 - Secure Downloader for AyhanOS\n");
        fprintf(stderr, "Usage: %s <URL> [OutputFile] [Options]\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -O        Save with remote filename (like wget -O)\n");
        fprintf(stderr, "  -c        Resume partial download\n");
        fprintf(stderr, "  -k        Insecure (ignore SSL certificate errors)\n");
        fprintf(stderr, "  -q        Quiet mode (no progress bar)\n");
        fprintf(stderr, "  -x <proxy> Use proxy (e.g., socks5://127.0.0.1:1080)\n");
        return 1;
    }

    const char *url = argv[1];
    char out_filename[512] = {0};
    int resume = 0;
    int insecure = 0;
    int quiet = 0;
    int use_remote_name = 0;
    const char *proxy = NULL;

    // تحلیل آرگومان‌های اضافی
    int arg_index = 2;
    if (argc > 2 && argv[2][0] != '-') {
        strncpy(out_filename, argv[2], sizeof(out_filename) - 1);
        arg_index = 3;
    }

    for (int i = arg_index; i < argc; i++) {
        if (strcmp(argv[i], "-O") == 0) use_remote_name = 1;
        else if (strcmp(argv[i], "-c") == 0) resume = 1;
        else if (strcmp(argv[i], "-k") == 0) insecure = 1;
        else if (strcmp(argv[i], "-q") == 0) quiet = 1;
        else if (strcmp(argv[i], "-x") == 0 && i + 1 < argc) proxy = argv[++i];
    }

    // اگر نام فایل مشخص نشده، از URL استخراج کن
    if (strlen(out_filename) == 0) {
        get_filename_from_url(url, out_filename);
        use_remote_name = 1;
    }

    // بررسی HTTPS
    if (strncmp(url, "https://", 8) != 0) {
        fprintf(stderr, "[-] Error: AyhanOS requires HTTPS. HTTP is blocked.\n");
        return 1;
    }

    if (!quiet) {
        printf("[*] AyhanOSDL v3.0\n");
        printf("[*] URL: %s\n", url);
        printf("[*] Output: %s\n", out_filename);
        if (insecure) printf("[!] Warning: SSL certificate verification is disabled.\n");
    }

    CURL *curl;
    FILE *fp;
    CURLcode res;
    struct progress_data prog = {0};
    prog.quiet = quiet;

    // بررسی وجود فایل برای ادامه دانلود
    struct stat st;
    long resume_from = 0;
    if (resume && stat(out_filename, &st) == 0) {
        resume_from = st.st_size;
        fp = fopen(out_filename, "ab");
        if (!quiet) printf("[*] Resuming from byte: %ld\n", resume_from);
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
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "AyhanOSDL/3.0 (AyhanOS)");

        // نادیده گرفتن گواهی فقط اگر -k زده شده باشد
        if (insecure) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        // نوار پیشرفت
        if (!quiet) {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }

        // ادامه دانلود
        if (resume_from > 0) {
            curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)resume_from);
        }

        // پروکسی
        if (proxy) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
        }

        if (!quiet) printf("[*] Downloading...\n");
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            fprintf(stderr, "\n[-] Download failed: %s\n", curl_easy_strerror(res));
        } else {
            if (!quiet) printf("\n[+] Download successful: %s\n", out_filename);
        }

        curl_easy_cleanup(curl);
    }
    
    fclose(fp);
    curl_global_cleanup();
    return 0;
}
