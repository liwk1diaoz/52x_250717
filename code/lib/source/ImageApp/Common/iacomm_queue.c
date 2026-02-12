#include "ImageApp_Common_int.h"

///// Cross file variables
///// Noncross file variables

void _QUEUE_EnQueueToTail(QUEUE_BUFFER_QUEUE *queue, QUEUE_BUFFER_ELEMENT *element)
{
	vos_sem_wait(IACOMMON_SEMID_QUE);
	if (queue && element) {
		if (queue->size == 0) {
			queue->head = element;
			element->front = NULL;
			queue->tail = element;
			element->rear = NULL;
		} else {
			element->front = queue->tail;
			queue->tail->rear = element;
			queue->tail = element;
			element->rear = NULL;
		}
		queue->size++;
	}
	vos_sem_sig(IACOMMON_SEMID_QUE);
}

QUEUE_BUFFER_ELEMENT* _QUEUE_DeQueueFromHead(QUEUE_BUFFER_QUEUE *queue)
{
	QUEUE_BUFFER_ELEMENT *retElement = NULL;

	vos_sem_wait(IACOMMON_SEMID_QUE);
	if (queue && queue->size != 0) {
		retElement = queue->head;
		queue->head = retElement ? retElement->rear : 0;
		if (queue->head) {
			queue->head->front = NULL;
		}
		if (retElement) {
			retElement->rear = NULL;
		}
		queue->size--;
	}
	vos_sem_sig(IACOMMON_SEMID_QUE);
	return retElement;
}

UINT32 _QUEUE_GetQueueSize(QUEUE_BUFFER_QUEUE *queue)
{
	return queue ? queue->size : 0;
}

