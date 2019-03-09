/* -*- Mode: C; indent-tabs-mode: nil; 
   c-basic-offset: 8 c-style: "K&R" -*- */
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* 1999-02-22 Arkadiusz Mié∂kiewicz <misiek@misiek.eu.org>
 * - added Native Language Support
 */

/* 2002-04-17 Satoru Takabayashi <satoru@namazu.org>
 * - modify `script' to create `chatty'.
 */

/*
 * script
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <stdio.h>

#if defined(SVR4)
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#endif /* SVR4 */

#ifdef __linux__
#include <unistd.h>
#include <string.h>
#endif

#include <sys/time.h>
#include "dict.h"

#define HAVE_inet_aton
#define HAVE_scsi_h
#define HAVE_kd_h

#define _(FOO) FOO

#ifdef HAVE_openpty
#include <pty.h>  /* for openpty and forkpty */
#include <utmp.h> /* for login_tty */
#endif

#include <errno.h>

#if defined(SVR4) && !defined(CDEL)
#if defined(_POSIX_VDISABLE)
#define CDEL _POSIX_VDISABLE
#elif defined(CDISABLE)
#define CDEL CDISABLE
#else /* not _POSIX_VISIBLE && not CDISABLE */
#define CDEL 255
#endif /* not _POSIX_VISIBLE && not CDISABLE */
#endif /* SVR4 && ! CDEL */

void done(void);
void fail(void);
void fixtty(void);
void getmaster(void);
void getslave(void);
void doinput(const char*);
void dooutput(void);
void doshell(const char*);

char	*shell;
int	master;
int	slave;
int	child;
int	subchild;
char	*fname;

struct	termios tt;
struct	winsize win;
int	lb;
int	l;
#if !defined(SVR4)
#ifndef HAVE_openpty
char	line[] = "/dev/ptyXX";
#endif
#endif /* !SVR4 */


#include <stdlib.h>
typedef struct {
    char str[BUFSIZ];
    char *top;
    char *tail;
} RevBuf;

Dict*	read_dict		(const char *file_name);
RevBuf*	revbuf_new		(void);
void	revbuf_add		(RevBuf *revbuf, int ch);
void	revbuf_clear		(RevBuf *revbuf);
char*	revbuf_top		(RevBuf *revbuf);
int	revbuf_full_p		(RevBuf *revbuf);
void	watch_input		(Dict *dict, RevBuf *revbuf,
				 const char *str, int len);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	int ch;
	void finish();
	char *getenv();
	char *command = NULL;

	while ((ch = getopt(argc, argv, "h?")) != EOF)
		switch((char)ch) {
		case 'h':
		case '?':
		default:
			fprintf(stderr, _("usage: chatty <dict>\n"));
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
	    fprintf(stderr, _("usage: chatty <dict>\n"));
	    exit(1);
	}

	shell = getenv("SHELL");
	if (shell == NULL)
		shell = "/bin/sh";

	getmaster();
	fixtty();

	(void) signal(SIGCHLD, finish);
	child = fork();
	if (child < 0) {
		perror("fork");
		fail();
	}
	if (child == 0) {
		subchild = child = fork();
		if (child < 0) {
			perror("fork");
			fail();
		}
		if (child)
			dooutput();
		else
			doshell(command);
	}
	doinput(argv[0]);

	return 0;
}

char *
reverse (char *str)
{
    int i, len = strlen(str);

    str = strdup(str);
    for (i = 0; i < len / 2; i++) {
	char w = str[i];
	str[i] = str[len - i - 1];
	str[len - i - 1] = w;
    }
    return str;
}

Dict *
read_dict (const char *file_name)
{
    char buf[BUFSIZ];
    Dict *dict = dict_new();
    FILE *fp = fopen(file_name, "r");

    if (fp == NULL) {
	fprintf(stderr, "chatty: %s: %s\n", file_name, strerror(errno));
	exit(1);
    }

    while (fgets(buf, BUFSIZ, fp)) {
	char *tabpos;
	buf[strlen(buf)-1] = '\0';
	tabpos = strchr(buf, '\t');
	if (tabpos) {
	    char *key, value[BUFSIZ];
	    *tabpos = '\0';
	    key = buf;
	    if (strlen(key) > 0) {
		char *rkey = reverse(key);
		sprintf(value, "%s: %s", key, tabpos + 1);
		dict_add(dict, rkey, value);
		free(rkey);
	    }
	}
    }
    fclose(fp);
    return dict;
}

RevBuf *
revbuf_new (void)
{
    RevBuf *revbuf = malloc(sizeof(RevBuf));
    if (revbuf == NULL)
	abort();
    revbuf->tail = revbuf->str + BUFSIZ - 1;
    *(revbuf->tail) = '\0';
    revbuf->top = revbuf->tail;
    return revbuf;
}

void
revbuf_add (RevBuf *revbuf, int ch)
{
    revbuf->top--;
    *(revbuf->top) = ch;
}

int
revbuf_full_p (RevBuf *revbuf)
{
    return revbuf->top == revbuf->str;
}

char *
revbuf_top (RevBuf *revbuf)
{
    return revbuf->top;
}

void
revbuf_clear (RevBuf *revbuf)
{
    revbuf->top = revbuf->tail;
}

void
watch_input (Dict *dict, RevBuf *revbuf, const char *str, int len)
{
    int i;

    for (i = 0; i < len; i++) {
	int  ch = str[i];
	char *message;
	if (revbuf_full_p(revbuf))
	    revbuf_clear(revbuf);
	revbuf_add(revbuf, ch);
	message = dict_search_longest(dict, revbuf_top(revbuf));
	if (message)
	    printf("\033]0;%s\a", message);
	if (ch == '\n' || ch == '\r')
	    revbuf_clear(revbuf);
    }
}

void
doinput(const char *file_name)
{
	register int cc;
	char ibuf[BUFSIZ];

	Dict*	dict   = read_dict(file_name);
	RevBuf*	revbuf = revbuf_new();

	setbuf(stdout, NULL);
#ifdef HAVE_openpty
	(void) close(slave);
#endif
	while ((cc = read(0, ibuf, BUFSIZ)) > 0) {
		watch_input(dict, revbuf, ibuf, cc);
		(void) write(master, ibuf, cc);
	}
	done();
}

#include <sys/wait.h>

void
finish()
{
#if defined(SVR4)
	int status;
#else /* !SVR4 */
	union wait status;
#endif /* !SVR4 */
	register int pid;
	register int die = 0;

	while ((pid = wait3((int *)&status, WNOHANG, 0)) > 0)
		if (pid == child)
			die = 1;

	if (die)
		done();
}

void
dooutput()
{
	int cc;
	time_t tvec, time();
	char obuf[BUFSIZ], *ctime();

	setbuf(stdout, NULL);
	(void) close(0);
#ifdef HAVE_openpty
	(void) close(slave);
#endif
	tvec = time((time_t *)NULL);
	for (;;) {
		cc = read(master, obuf, BUFSIZ);
		if (cc <= 0)
			break;
		(void) write(1, obuf, cc);
	}

	done();
}

void
doshell(const char* command)
{
	/***
	int t;

	t = open(_PATH_TTY, O_RDWR);
	if (t >= 0) {
		(void) ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	}
	***/
	getslave();
	(void) close(master);
	(void) dup2(slave, 0);
	(void) dup2(slave, 1);
	(void) dup2(slave, 2);
	(void) close(slave);

	if (!command) {
		execl(shell, strrchr(shell, '/') + 1, "-i", 0);
	} else {
		execl(shell, strrchr(shell, '/') + 1, "-c", command, 0);	
	}
	perror(shell);
	fail();
}

void
fixtty()
{
	struct termios rtt;

	rtt = tt;
#if defined(SVR4)
	rtt.c_iflag = 0;
	rtt.c_lflag &= ~(ISIG|ICANON|XCASE|ECHO|ECHOE|ECHOK|ECHONL);
	rtt.c_oflag = OPOST;
	rtt.c_cc[VINTR] = CDEL;
	rtt.c_cc[VQUIT] = CDEL;
	rtt.c_cc[VERASE] = CDEL;
	rtt.c_cc[VKILL] = CDEL;
	rtt.c_cc[VEOF] = 1;
	rtt.c_cc[VEOL] = 0;
#else /* !SVR4 */
	cfmakeraw(&rtt);
	rtt.c_lflag &= ~ECHO;
#endif /* !SVR4 */
	(void) tcsetattr(0, TCSAFLUSH, &rtt);
}

void
fail()
{
	(void) kill(0, SIGTERM);
	done();
}

void
done()
{
	time_t tvec, time();
	char *ctime();

	if (subchild) {
		tvec = time((time_t *)NULL);
		(void) close(master);
	} else {
		(void) tcsetattr(0, TCSAFLUSH, &tt);
	}
	exit(0);
}

void
getmaster()
{
#if defined(SVR4)
	(void) tcgetattr(0, &tt);
	(void) ioctl(0, TIOCGWINSZ, (char *)&win);
	if ((master = open("/dev/ptmx", O_RDWR)) < 0) {
		perror("open(\"/dev/ptmx\", O_RDWR)");
		fail();
	}
#else /* !SVR4 */
#ifdef HAVE_openpty
	(void) tcgetattr(0, &tt);
	(void) ioctl(0, TIOCGWINSZ, (char *)&win);
	if (openpty(&master, &slave, NULL, &tt, &win) < 0) {
		fprintf(stderr, _("openpty failed\n"));
		fail();
	}
#else
	char *pty, *bank, *cp;
	struct stat stb;

	pty = &line[strlen("/dev/ptyp")];
	for (bank = "pqrs"; *bank; bank++) {
		line[strlen("/dev/pty")] = *bank;
		*pty = '0';
		if (stat(line, &stb) < 0)
			break;
		for (cp = "0123456789abcdef"; *cp; cp++) {
			*pty = *cp;
			master = open(line, O_RDWR);
			if (master >= 0) {
				char *tp = &line[strlen("/dev/")];
				int ok;

				/* verify slave side is usable */
				*tp = 't';
				ok = access(line, R_OK|W_OK) == 0;
				*tp = 'p';
				if (ok) {
					(void) tcgetattr(0, &tt);
				    	(void) ioctl(0, TIOCGWINSZ, 
						(char *)&win);
					return;
				}
				(void) close(master);
			}
		}
	}
	fprintf(stderr, _("Out of pty's\n"));
	fail();
#endif /* not HAVE_openpty */
#endif /* !SVR4 */
}

void
getslave()
{
#if defined(SVR4)
	(void) setsid();
	grantpt( master);
	unlockpt(master);
	if ((slave = open((const char *)ptsname(master), O_RDWR)) < 0) {
		perror("open(fd, O_RDWR)");
		fail();
	}
	if (isastream(slave)) {
		if (ioctl(slave, I_PUSH, "ptem") < 0) {
			perror("ioctl(fd, I_PUSH, ptem)");
			fail();
		}
		if (ioctl(slave, I_PUSH, "ldterm") < 0) {
			perror("ioctl(fd, I_PUSH, ldterm)");
			fail();
		}
#ifndef _HPUX_SOURCE
		if (ioctl(slave, I_PUSH, "ttcompat") < 0) {
			perror("ioctl(fd, I_PUSH, ttcompat)");
			fail();
		}
#endif
		(void) ioctl(0, TIOCGWINSZ, (char *)&win);
	}
#else /* !SVR4 */
#ifndef HAVE_openpty
	line[strlen("/dev/")] = 't';
	slave = open(line, O_RDWR);
	if (slave < 0) {
		perror(line);
		fail();
	}
	(void) tcsetattr(slave, TCSAFLUSH, &tt);
	(void) ioctl(slave, TIOCSWINSZ, (char *)&win);
#endif
	(void) setsid();
	(void) ioctl(slave, TIOCSCTTY, 0);
#endif /* SVR4 */
}
