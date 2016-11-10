#ifndef CONFIGPARSE_H_
#define CONFIGPARSE_H_

struct config {
    char server[20];
    char username[20];
    char password[20];
    unsigned char CONTROLCHECKSTATUS;
    unsigned char ADAPTERNUM;
    char host_ip[20];
    unsigned char IPDOG;
    char host_name[20];
    char PRIMARY_DNS[20];
    char dhcp_server[20];
    unsigned char AUTH_VERSION[2];
    unsigned char mac[6];
    char host_os[20];
    unsigned char KEEP_ALIVE_VERSION[2];
    int ror_version;
    unsigned char pppoe_flag;
    unsigned char keep_alive2_flag; /* abandoned */
};

extern struct config drcom_config;
extern int verbose_flag;
extern int logging_flag;
extern char *log_path;
extern char *mode;

int config_parse(char *filepath, char *mode);

#endif // CONFIGPARSE_H_