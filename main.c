#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "auth.h"
#include "configparse.h"

#ifdef linux
#include <limits.h>
#include "daemon.h"
#include "eapol.h"
#include "libs/common.h"
#endif

#define VERSION "1.6.2"

void print_help(int exval);
int try_smart_eaplogin(void);

static const char default_bind_ip[20] = "0.0.0.0";
static const char default_pid_path[20] = "/tmp/drcom.pid";
int main(int argc, char *argv[]) {
    if (argc == 1) {
        print_help(1);
    }

    char *file_path;
    char path_tmp[PATH_MAX];

    while (1) {
        static const struct option long_options[] = {
            {"mode", required_argument, 0, 'm'},
            {"conf", required_argument, 0, 'c'},
            {"bindip", required_argument, 0, 'b'},
            {"log", required_argument, 0, 'l'},
#ifdef linux
            {"daemon", no_argument, 0, 'd'},
            {"pid", required_argument, 0, 'p'},
            {"802.1x", no_argument, 0, 'x'},
#endif
            {"eternal", no_argument, 0, 'e'},
            {"verbose", no_argument, 0, 'v'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};

        int c;
        int option_index = 0;
#ifdef linux
        c = getopt_long(argc, argv, "m:c:b:l:p:dxevh", long_options, &option_index);
#else
        c = getopt_long(argc, argv, "m:c:b:l:evh", long_options, &option_index);
#endif

        if (c == -1) {
            break;
        }
        switch (c) {
            case 'm':
                if (strcmp(optarg, "dhcp") == 0) {
                    strcpy(mode, optarg);
                } else if (strcmp(optarg, "pppoe") == 0) {
                    strcpy(mode, optarg);
                } else {
                    printf("unknown mode\n");
                    exit(1);
                }
                break;
            case 'c':
#ifndef __APPLE__
                if (mode != NULL) {
#endif
#ifdef linux
                    memset(path_tmp,'\0',sizeof(path_tmp));
                    realpath(optarg, path_tmp);
                    if(strlen(path_tmp) == 0){
                      file_path = strdup(optarg);
                    }
                    else{
                      file_path = strdup(path_tmp);
                    }
#else
                file_path = optarg;
#endif
#ifndef __APPLE__
                }
#endif
                break;
            case 'b':
                strcpy(bind_ip, optarg);
                break;
            case 'l':
#ifndef __APPLE__
                if (mode != NULL) {
#endif
#ifdef linux
                    memset(path_tmp,'\0',sizeof(path_tmp));
                    realpath(optarg, path_tmp);
                    if(strlen(path_tmp)==0){
                      log_path = strdup(optarg);
                    }
                    else{
                      log_path = strdup(path_tmp);
                    }
#else
                log_path = optarg;
#endif
                    logging_flag = 1;
#ifndef __APPLE__
                }
#endif
                break;
#ifdef linux
            case 'd':
                daemon_flag = 1;
                break;
            case 'p':
                if (mode != NULL) {
                    memset(path_tmp,'\0',sizeof(path_tmp));
                    realpath(optarg, path_tmp);
                    if(strlen(path_tmp)==0){
                      pid_path = strdup(optarg);
                    }
                    else{
                      pid_path = strdup(path_tmp);
                    }
                    daemon_flag = 1;
                }
                break;
            case 'x':
                eapol_flag = 1;
                break;
#endif
            case 'e':
                eternal_flag = 1;
                break;
            case 'v':
                verbose_flag = 1;
                break;
            case 'h':
                print_help(0);
                break;
            case '?':
                print_help(1);
                break;
            default:
                break;
        }
    }

#ifndef __APPLE__
    if (mode != NULL && file_path != NULL) {
#endif
#ifdef linux
        if (daemon_flag) {
            if (pid_path == NULL || strlen(pid_path)==0){
                pid_path = strdup(default_pid_path);
            }
            daemonise();
        }
#endif

#ifdef WIN32  // dirty fix with win32
        char tmp[10] = {0};
        strcpy(tmp, mode);
#endif
        if (!config_parse(file_path)) {
#ifdef WIN32  // dirty fix with win32
            strcpy(mode, tmp);
#endif

#ifdef linux
            if (eapol_flag) {  // eable 802.1x authorization
                if (0 != try_smart_eaplogin()) {
                    printf("Can't finish 802.1x authorization!\n");
                    return 1;
                }
            }
#endif
            if (strlen(bind_ip) == 0) {
                memcpy(bind_ip, default_bind_ip, sizeof(default_bind_ip));
            }
            dogcom(5);
        } else {
            return 1;
        }
#ifndef __APPLE__
    } else {
        printf("Need more options!\n\n");
        return 1;
    }
#endif
    return 0;
}

void print_help(int exval) {
    printf("\nDrcom-generic implementation in C.\n");
    printf("Version: %s\n\n", VERSION);

    printf("Usage:\n");
    printf("\tdogcom -m <dhcp/pppoe> -c <FILEPATH> [options <argument>]...\n\n");

    printf("Options:\n");
    printf("\t--mode <dhcp/pppoe>, -m <dhcp/pppoe>  set your dogcom mode \n");
    printf("\t--conf <FILEPATH>, -c <FILEPATH>      import configuration file\n");
    printf("\t--bindip <IPADDR>, -b <IPADDR>        bind your ip address(default is 0.0.0.0)\n");
    printf("\t--log <LOGPATH>, -l <LOGPATH>         specify log file\n");
#ifdef linux
    printf("\t--daemon, -d                          set daemon flag\n");
    printf("\t--pid <PIDPATH>, -p <PIDPATH>         specify pid location(default is /tmp/drcom.pid)\n");
    printf("\t--802.1x, -x                          enable 802.1x\n");
#endif
    printf("\t--eternal, -e                         set eternal flag\n");
    printf("\t--verbose, -v                         set verbose flag\n");
    printf("\t--help, -h                            display this help\n\n");
    exit(exval);
}

#ifdef linux
int try_smart_eaplogin(void) {
#define IFS_MAX (64)
    int ifcnt = IFS_MAX;
    iflist_t ifs[IFS_MAX];
    if (0 > getall_ifs(ifs, &ifcnt))
        return -1;

    for (int i = 0; i < ifcnt; ++i) {
        setifname(ifs[i].name);
        if (0 == eaplogin(drcom_config.username, drcom_config.password))
            return 0;
    }
    return -1;
}
#endif