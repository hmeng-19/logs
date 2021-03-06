#define _GNU_SOURCE
#include <sys/capability.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#define errExit(msg)	do { perror(msg); exit(EXIT_FAILURE); \
						} while (0)

static void
usage(char *pname)
{
	fprintf(stderr, "Usage: %s [options] <pid>\n\n", pname);
	fprintf(stderr, "joins the user namespace of another process <pid>," 
			"unshares mount|uts|pid|ipc|network namespaces,\n",
			"and executes `sh`\n\n");
	fprintf(stderr, "Options can be:\n\n");
#define fpe(str) fprintf(stderr, "	%s", str);
	fpe("-i		  New IPC namespace\n");
	fpe("-m		  New mount namespace\n");
	fpe("-n		  New network namespace\n");
	fpe("-p		  New PID namespace\n");
	fpe("-u		  New UTS namespace\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	cap_t caps;
	pid_t pid;
	char *existing_pid;
	char path[PATH_MAX];
	int fd;
	int flags, opt;
	int r;

	while ((opt = getopt(argc, argv, "+imnpuh")) != -1) {
		switch (opt) {
		case 'i': flags |= CLONE_NEWIPC;		break;
		case 'm': flags |= CLONE_NEWNS;		 break;
		case 'n': flags |= CLONE_NEWNET;		break;
		case 'p': flags |= CLONE_NEWPID;		break;
		case 'u': flags |= CLONE_NEWUTS;		break;
		default:  usage(argv[0]);
		}
	}

	existing_pid = argv[optind];
	
	/* Create child; child commences execution in childFunc() */
	snprintf(path, sizeof(path), "/proc/%s/ns/user", existing_pid);

	fd = open(path, O_RDONLY);
	if(fd == -1) {
		printf("open(%s) failed: %s\n", path, strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("******* info of the parent process - start ********\n");
	caps = cap_get_proc();
	printf("before joining user namespace of process %s, \ncapabilities: %s\n\n", existing_pid, cap_to_text(caps, NULL));
	
	if(setns(fd, 0) == -1) {
		printf("setns(%d) failed: %s\n", fd, strerror(errno));
		close(fd);
		exit(EXIT_FAILURE);
	}
	close(fd);

	caps = cap_get_proc();
	printf("after joining user namespace of process %s, \ncapabilities: %s\n\n", existing_pid, cap_to_text(caps, NULL));

	printf("the pid is: %ld\n", (long)getpid());

	r = unshare(flags);
	if(r == -1) {
		printf("unshare failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	r = execlp("sh", "sh", (char *)0);
	if(r == -1) {
		printf("execlp failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	



	exit(EXIT_SUCCESS);
}
