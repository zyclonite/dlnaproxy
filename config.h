/* DLNAProxy Project
 * (c) 2011 Lukas Prettenthaler
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define OS_NAME			"Linux"
#define OS_VERSION		"Linux/2.6.x"
#define OS_URL			"http://zyclonite.net/"

/* full path of the log directory */
#define DEFAULT_LOG_PATH	"/var/log"

/* Comment the following line to use home made daemonize() func instead
 * of BSD daemon() */
#define USE_DAEMON

/* Enable if the system inotify.h exists.  Otherwise our own inotify.h will be used. */
/*#define HAVE_INOTIFY_H*/

#endif
