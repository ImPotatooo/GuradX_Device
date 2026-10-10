/* GuardX RPi B fire-decision core. */
#ifndef GUARDX_ERR_H
#define GUARDX_ERR_H

typedef enum {
    GUARDX_OK            = 0,
    GUARDX_ERR_OPEN      = -1,
    GUARDX_ERR_READ      = -2,
    GUARDX_ERR_CLOSE     = -3,
    GUARDX_ERR_NOT_OPEN  = -4,
    GUARDX_ERR_INVALID   = -5,
    GUARDX_ERR_WRITE     = -6,
} guardx_err_t;

#endif
