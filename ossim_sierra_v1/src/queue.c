#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if (!q || q->size >= MAX_QUEUE_SIZE) return; // bảo vệ overflow
        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (!q || q->size == 0) return NULL;

        struct pcb_t * p = q->proc[0];
    
        // Dịch mảng về bên trái để giữ thứ tự
        for (int i = 1; i < q->size; i++) {
            q->proc[i - 1] = q->proc[i];
        }
    
        q->size--;
        return p;
}

// //Huy gửi
// #include <stdio.h>
// #include <stdlib.h>
// #include "queue.h"

// int empty(struct queue_t * q) {
//         if (q == NULL) return 1;
// 	return (q->size == 0);
// }

// void enqueue(struct queue_t * q, struct pcb_t * proc) {
// 	/* TODO: put a new process to queue [q] */	
// 	if (proc == NULL || q == NULL|| q->size == MAX_QUEUE_SIZE) return;
// 	q->proc[q->size] = proc;
// 	q->size += 1;
// }

// struct pcb_t * dequeue(struct queue_t * q) {
// 	/* TODO: return a pcb whose prioprity is the highest
// 	 * in the queue [q] and remember to remove it from q
// 	 * */
// 	if (empty(q)) {
// 		return NULL;
// 	} else {
// 		struct pcb_t * temp = q->proc[0];
// 		for (int i = 0; i < q->size - 1; i++) {
// 			q->proc[i] = q->proc[i + 1];
// 		}
// 		q->proc[q->size] = NULL;
// 		q->size--;
// 		return temp;
// 	}
	
// }