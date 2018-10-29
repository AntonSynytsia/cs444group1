static int clook_dispatch(struct request_queue *q, int force)
{
    struct clook_data *nd = q->elevator->elevator_data;

    if (!list_empty(&nd->queue)) {
        struct request *rq;
        rq = list_entry(nd->queue.next, struct request, queuelist);
        if (g_last_seg > blk_rq_pos(rq)) {
            printk("UNEXPECTED CLOOK ACCESS %llu after %llu\n", blk_rq_pos(rq), g_last_seg);
        }
        list_del_init(&rq->queuelist);
        elv_dispatch_sort(q, rq);
        return 1;
    }
    else {
        g_last_seg = 0;
    }
    return 0;
}
