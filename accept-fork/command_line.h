
/* COMMAND LINE OPTIONS */
#define  o_BIND       (1)
#define  o_CONF       (2)
#define  o_DOCUMENTS  (3)
#define  o_EXEC       (4)
#define  o_FOREGROUND (5)
#define  o_HELP       (6)
#define  o_LOG        (7)
#define  o_PORT       (8)
#define  o_SOCKET     (9)
#define  o_UDS       (10)
#define  o_USER      (11)
#define  o_VERBOSE   (12)
#define  o_VERSION   (13)
#define  o_WORKERS   (14)

#define WORKERS_DEFAULT (5)
#define WORKERS_MAX (15)
#define MAX_ENVS (50)



extern char * server_ip;
extern char * document_root;
extern int    foreground_flag;
extern char * port;
extern int    port_num;
extern char * scriptname;
extern char * socket_file;
extern int    user;
extern int    verbose_flag;
extern int    workers;
extern int    help_flag;
extern int    version_flag;

extern void set_command_line_options(int argc, char *argv[], char *scgi_home);
