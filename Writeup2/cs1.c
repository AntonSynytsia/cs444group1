static void clook_add_request(struct request_queue *q, struct request *rq)
{
    struct clook_data *nd = q->elevator->elevator_data;

    struct list_head *head = &nd->queue;
    struct list_head *item = &rq->queuelist;

    // Find node to append to
    struct list_head *node = head;
    struct list_head *next_node = head->next;

    while (next_node != head) { // while node is not last
        // get request at next node
        struct request *nrq = list_entry(next_node, struct request, queuelist);

        // if request at next node is greater than rq, break out
        if (blk_rq_pos(nrq) > blk_rq_pos(rq)) break;

        // increment
        node = next_node;
        next_node = node->next;
    }

    // Append request item to node
    list_add(item, node);
}
