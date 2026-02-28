#ifndef ERRNO_H
#define ERRNO_H

extern int errno;

#define E2BIG 7            /* Argument list too long */
#define EACCES 13          /* Permission denied */
#define EADDRINUSE 98      /* Address already in use */
#define EADDRNOTAVAIL 99   /* Cannot assign requested address */
#define EAFNOSUPPORT 97    /* Address family not supported by protocol */
#define EAGAIN 11          /* Resource temporarily unavailable */
#define EALREADY 114       /* Connection already in progress */
#define EBADF 9            /* Bad file descriptor */
#define EBADMSG 74         /* Bad message */
#define EBUSY 16           /* Device or resource busy */
#define ECANCELED          /* platform specific — not always standard */
#define ECHILD 10          /* No child processes */
#define ECONNABORTED 103   /* Software caused connection abort */
#define ECONNREFUSED 111   /* Connection refused */
#define ECONNRESET 104     /* Connection reset by peer */
#define EDEADLK 35         /* Resource deadlock avoided */
#define EDESTADDRREQ 89    /* Destination address required */
#define EDOM 33            /* Mathematics argument out of domain */
#define EDQUOT 122         /* Reserved / Disk quota exceeded */
#define EEXIST 17          /* File exists */
#define EFAULT 14          /* Bad address */
#define EFBIG 27           /* File too large */
#define EHOSTUNREACH 113   /* No route to host */
#define EIDRM 43           /* Identifier removed */
#define EILSEQ 84          /* Illegal byte sequence */
#define EINPROGRESS 115    /* Operation now in progress */
#define EINTR 4            /* Interrupted system call */
#define EINVAL 22          /* Invalid argument */
#define EIO 5              /* I/O error */
#define EISCONN 106        /* Transport endpoint is already connected */
#define EISDIR 21          /* Is a directory */
#define ELOOP 40           /* Too many levels of symbolic links */
#define EMFILE 24          /* Too many open files */
#define EMLINK 31          /* Too many links */
#define EMSGSIZE 90        /* Message too long */
#define EMULTIHOP 72       /* Multihop attempted */
#define ENAMETOOLONG 36    /* File name too long */
#define ENETDOWN 100       /* Network is down */
#define ENETRESET 102      /* Network dropped connection on reset */
#define ENETUNREACH 101    /* Network is unreachable */
#define ENFILE 23          /* Too many open files in system */
#define ENOBUFS 105        /* No buffer space available */
#define ENODATA 61         /* No data available on STREAM head */
#define ENODEV 19          /* No such device */
#define ENOENT 2           /* No such file or directory */
#define ENOEXEC 8          /* Exec format error */
#define ENOLCK 37          /* No locks available */
#define ENOLINK 67         /* Link has been severed */
#define ENOMEM 12          /* Cannot allocate memory */
#define ENOMSG 42          /* No message of the desired type */
#define ENOPROTOOPT 92     /* Protocol not available */
#define ENOSPC 28          /* No space left on device */
#define ENOSR 63           /* No STREAM resources */
#define ENOSTR 60          /* Not a STREAM */
#define ENOSYS 38          /* Function not implemented */
#define ENOTCONN 107       /* Transport endpoint is not connected */
#define ENOTDIR 20         /* Not a directory */
#define ENOTEMPTY 39       /* Directory not empty */
#define ENOTSOCK 88        /* Socket operation on non-socket */
#define ENOTSUP 95         /* Operation not supported */
#define ENOTTY 25          /* Inappropriate I/O control operation */
#define ENXIO 6            /* No such device or address */
#define EOPNOTSUPP 95      /* Operation not supported on transport endpoint */
#define EOVERFLOW 75       /* Value too large to be stored in data type */
#define EPERM 1            /* Operation not permitted */
#define EPIPE 32           /* Broken pipe */
#define EPROTO 71          /* Protocol error */
#define EPROTONOSUPPORT 93 /* Protocol not supported */
#define EPROTOTYPE 91      /* Protocol wrong type for socket */
#define ERANGE 34          /* Result too large */
#define EROFS 30           /* Read-only file system */
#define ESPIPE 29          /* Invalid seek */
#define ESRCH 3            /* No such process */
#define ESTALE 116         /* Stale file handle */
#define ETIME 62           /* Timer expired / Stream ioctl timeout */
#define ETIMEDOUT 110      /* Connection timed out */
#define ETXTBSY 26         /* Text file busy */
#define EWOULDBLOCK 11     /* Operation would block (same as EAGAIN) */
#define EXDEV 18           /* Invalid cross-device link */

/* these are not part of the standard */
#define EWRONGSIZE                                                             \
    300 /* invalid size, used for reads that need to read a certain amount of  \
           data */

#endif
