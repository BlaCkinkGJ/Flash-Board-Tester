/*
The MIT License (MIT)

Copyright (c) 2014-2015 CSAIL, MIT

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include <libmemio.h>

#include "include/lower.h"
#include "include/settings.h"

#define WRITE_SIZE (MAX_BLOCKS)

pthread_mutex_t mutex;

static bool badsegment[MAX_BLOCKS];

static void erase_end_req(uint64_t seg_num, uint8_t isbad) {
  /*managing block mapping when "isbad" set to 1 which mean the segments has
   * some bad blocks*/
  pthread_mutex_unlock(&mutex);
  if (isbad) {
    printf("bad segment detected %lu\n", seg_num);
    badsegment[seg_num] = true;
  }
}

static char buffer[PAGE_SIZE];

void generate_sample_data(char *buffer, int seed) {
  srand(seed * time(NULL));
  for (int i = 0; i < PAGE_SIZE; i++) {
    buffer[i] = rand() % CHAR_MAX;
  }
}

int main(int argc, char **argv) {
  memio_t *mio;
  struct address addr;
  FILE *fp;

  memset(badsegment, false, sizeof(badsegment));

  pthread_mutex_init(&mutex, NULL);

  if ((mio = memio_open()) == NULL) {
    printf("could not open memio\n");
    return -1;
  }
  printf("successfully open\n");
  // trim and badsegment checking
  for (uint32_t block = 0; block < WRITE_SIZE; block++) {
    pthread_mutex_lock(&mutex);
    addr.lpn = block * PAGE_PER_SEGMENT;
    memio_trim(mio, addr.lpn, PAGE_PER_SEGMENT * PAGE_SIZE, erase_end_req);
  }
  printf("trim the segment\n");

  pthread_mutex_lock(&mutex);
  // R/W tester
  printf("I/O start\n");
  flashinit();
  for (uint32_t block = 0; block < WRITE_SIZE; block++) {
    printf("block %u\n", block);
    for (uint32_t i = 0; i < PAGE_PER_SEGMENT; i++) {
      addr.lpn = block * PAGE_PER_SEGMENT + i;
      if (badsegment[addr.lpn / PAGE_PER_SEGMENT]) {
        continue;
      }
      generate_sample_data(buffer, addr.lpn);
      flashwrite(mio, buffer, addr.lpn);
      flashread(mio, addr.lpn);
    }
    flashupdate(badsegment);
  }
  printf("finish I/O\n");

  fp = fopen("badblock.log", "w");
  addr.lpn = 0;
  for (uint32_t block = 0; block < WRITE_SIZE; block++) {
    if (!badsegment[block]) {
      continue;
    }
    fprintf(fp, "%u\n", block);
  }
  fclose(fp);

  memio_close(mio);
  flashfree(badsegment);

  return 0;
}
