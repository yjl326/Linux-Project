#include "fifo.h"

/**
 * @brief 向fifo中写入数据
 * 
 * @param fifo fifo 句柄
 * @param data 待写入数据
 * @param len 待写入数据长度
 * @return uint16_t 实际写入数据长度
 */

uint16_t write_fifo(fifo_t *fifo, uint8_t *data, uint16_t len)
{
	uint16_t w_len = len;
	if(len >= fifo->fifo_len) {
		memcpy(fifo->buff, data + len - fifo->fifo_len, fifo->fifo_len);
		fifo->read_index = 0;
		fifo->write_index = fifo->fifo_len;
		fifo->data_len = fifo->fifo_len;
		w_len = fifo->fifo_len;
	} 
	else {
		if(fifo->data_len + len > fifo->fifo_len) {
			return 0;
		}

		if(fifo->write_index + len > fifo->fifo_len) {
			uint16_t tail_len = (fifo->write_index + len) % fifo->fifo_len;
			memcpy(fifo->buff + fifo->write_index, data, len - tail_len);
			memcpy(fifo->buff, data + len - tail_len, tail_len);
			fifo->write_index = tail_len;
		} else {
			memcpy(fifo->buff + fifo->write_index, data, len);
			fifo->write_index += len;
		}

		if(fifo->data_len + len > fifo->fifo_len) {
			fifo->read_index = (fifo->read_index + fifo->data_len + len - fifo->fifo_len) % fifo->fifo_len;
			fifo->data_len = fifo->fifo_len;
		} else {
			fifo->data_len += len;
		}
	}
 
	return w_len;
}

/**
 * @brief 从fifo中读取数据
 * 
 * @param fifo fifo句柄
 * @param data 读取数据缓存
 * @param len 读取长度
 * @return uint16_t 实际读出数据长度
 */
uint16_t read_fifo(fifo_t *fifo, uint8_t *data, uint16_t len)
{
	uint16_t r_len = len;
	if(fifo->data_len < len)
		r_len = fifo->data_len;
	if(fifo->read_index + r_len > fifo->fifo_len) {
		uint16_t tail_len = (fifo->read_index + r_len) % fifo->fifo_len;
		memcpy(data, fifo->buff + fifo->read_index, r_len - tail_len);
		memcpy(data + r_len - tail_len, fifo->buff, tail_len);
		fifo->read_index = tail_len;
	} 
	else {
		memcpy(data, fifo->buff + fifo->read_index, r_len);
		fifo->read_index += r_len;
	}
 
	fifo->data_len -= r_len;
	if(0 == fifo->data_len) {
		fifo->read_index = 0;
		fifo->write_index = 0;
	}
 
	return r_len;
}

/**
 * @brief 创建一个fifo
 * 
 * @param fifo_buf_len fifo缓存buf的大小
 * @return fifo_t* fifo 句柄
 */
fifo_t *fifo_create(uint16_t fifo_buf_len)
{
	fifo_t *m_fifo;
	m_fifo = (fifo_t *)malloc(sizeof(fifo_t));
	m_fifo->buff = (uint8_t *)malloc(fifo_buf_len);
	if(NULL == m_fifo->buff){
		free(m_fifo);
		return NULL;
	}
	m_fifo->fifo_len = fifo_buf_len;
	m_fifo->data_len = 0;
	m_fifo->read_index = 0;
	m_fifo->write_index = 0;
 
	return m_fifo;
}
 
/**
 * @brief 释放一个fifo
 * 
 * @param fifo 待释放函数指针
 */
void fifo_release(fifo_t *fifo)
{
	free(fifo->buff);
	free(fifo);
}
