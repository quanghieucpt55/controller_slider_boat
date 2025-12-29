#include "string.h"
#include "stdlib.h"
#include "queue.h"

static inline void __attribute__((nonnull, always_inline)) inc_idx(uint16_t *  pIdx,  uint16_t end,  uint16_t start)
{
//	(*pIdx)++;
//	*pIdx %= end;
	if (*pIdx < end - 1)
        { (*pIdx)++; }
	else
        { *pIdx = start; }
}

/*!	\brief Decrement index
**	\details Decrement buffer index \b pIdx rolling back to \b end when limit \b start is reached
**	\param [in,out] pIdx - pointer to index value
**	\param [in] end - counter upper limit value
**	\param [in] start - counter lower limit value
**/
static inline void __attribute__((nonnull, always_inline)) dec_idx(uint16_t *  pIdx,  uint16_t end,  uint16_t start)
{
	if (*pIdx > start)		{ (*pIdx)--; }
	else					{ *pIdx = end - 1; }
}
void * __attribute__((nonnull)) q_init(Queue_t *  q,  uint16_t size_rec,  uint16_t nb_recs,  QueueType type,  uint8_t overwrite)
{
    uint32_t size = nb_recs * size_rec;

	q->rec_nb = nb_recs;
	q->rec_sz = size_rec;
	q->impl = type;
	q->ovw = overwrite;

	q_kill(q);	// Free existing data (if any)
	q->queue = (uint8_t *) malloc(size);

	if (q->queue == NULL)	{ q->queue_sz = 0; return 0; }	// Return here if Queue not allocated
	else					{ q->queue_sz = size; }

	q->init = QUEUE_INITIALIZED;
	q_flush(q);

	return q->queue;	// return NULL when queue not allocated (beside), Queue address otherwise
}
void __attribute__((nonnull)) q_kill(Queue_t *  q)
{
	if (q->init == QUEUE_INITIALIZED)
        { free(q->queue); }	// Free existing data (if already initialized)
	q->init = 0;
}
void __attribute__((nonnull)) q_flush(Queue_t *  q)
{
	q->in = 0;
	q->out = 0;
	q->cnt = 0;
}
uint8_t __attribute__((nonnull)) q_push(Queue_t *  q,  void * record)
{
	if ((!q->ovw) && q_isFull(q))
        { return false; }

	uint8_t * pStart = q->queue + (q->rec_sz * q->in);
	memcpy(pStart, record, q->rec_sz);

	inc_idx(&q->in, q->rec_nb, 0);

	if (!q_isFull(q))	{ q->cnt++; }	// Increase records count
	else if (q->ovw)					// Queue is full and overwrite is allowed
	{
		if (q->impl == FIFO)
            { inc_idx(&q->out, q->rec_nb, 0); }	// as oldest record is overwritten, increment out
		//else if (q->impl == LIFO)	{}										// Nothing to do in this case
	}

	return true;
}
uint8_t __attribute__((nonnull)) q_pop(Queue_t * q, void * record)
{
	const uint8_t * pStart;

	if (q_isEmpty(q))	{ return false; }	// No more records

	if (q->impl == FIFO)
	{
		pStart = q->queue + (q->rec_sz * q->out);
		inc_idx(&q->out, q->rec_nb, 0);
	}
	else if (q->impl == LIFO)
	{
		dec_idx(&q->in, q->rec_nb, 0);
		pStart = q->queue + (q->rec_sz * q->in);
	}
	else	{ return false; }

	memcpy(record, pStart, q->rec_sz);
	q->cnt--;	// Decrease records count
	return true;
}
uint8_t __attribute__((nonnull)) q_peek( Queue_t * q, void * record)
{
	const uint8_t * pStart;

	if (q_isEmpty(q))	{ return false; }	// No more records

	if (q->impl == FIFO)
	{
		pStart = q->queue + (q->rec_sz * q->out);
		// No change on out var as it's just a peek
	}
	else if (q->impl == LIFO)
	{
		uint16_t rec = q->in;	// Temporary var for peek (no change on q->in with dec_idx)
		dec_idx(&rec, q->rec_nb, 0);
		pStart = q->queue + (q->rec_sz * rec);
	}
	else	{ return false; }

	memcpy(record, pStart, q->rec_sz);
	return true;
}
uint8_t __attribute__((nonnull)) q_drop(Queue_t * q)
{
	if (q_isEmpty(q))			{ return false; }	// No more records

	if (q->impl == FIFO)		{ inc_idx(&q->out, q->rec_nb, 0); }
	else if (q->impl == LIFO)	{ dec_idx(&q->in, q->rec_nb, 0); }
	else						{ return false; }

	q->cnt--;	// Decrease records count
	return true;
}

uint8_t __attribute__((nonnull)) q_peekIdx( Queue_t *  q, void *  record, uint16_t idx)
{
	const uint8_t * pStart;

	if (idx + 1 > q_getCount(q))	{ return false; }	// Index out of range

	if (q->impl == FIFO)
	{
		pStart = q->queue + (q->rec_sz * ((q->out + idx) % q->rec_nb));
	}
	else if (q->impl == LIFO)
	{
		pStart = q->queue + (q->rec_sz * idx);
	}
	else	{ return false; }

	memcpy(record, pStart, q->rec_sz);
	return true;
}
