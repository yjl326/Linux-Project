#ifndef __FIFO_H
#define __FIFO_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define MAX_FIFO_SIZE 640 * 10

typedef struct _fifo_t
{
	uint8_t *buff;
	uint16_t fifo_len;
	uint16_t data_len;
	uint16_t read_index;
	uint16_t write_index;
}fifo_t;

uint16_t write_fifo(fifo_t *fifo, uint8_t *data, uint16_t len);
uint16_t read_fifo(fifo_t *fifo, uint8_t *data, uint16_t len);
fifo_t *fifo_create(uint16_t fifo_buf_len);
void fifo_release(fifo_t *fifo);

#endif
