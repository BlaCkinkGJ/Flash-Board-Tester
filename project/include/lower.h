#ifndef LOWER_H
#define LOWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libmemio.h>

// read/write test function
void flashinit(void);
void flashread(memio_t *mio, uint32_t lpn);
void flashwrite(memio_t *mio, char *buffer, uint32_t lpn);
void flashfree(void);

#ifdef __cplusplus
}
#endif

#endif