/*
 * Elevator clook
 */

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

sector_t g_last_seg = 0;

struct clook_data {
    struct list_head queue;
};

/*typedef struct {
    struct list_head* value;
    Node* prev;
    Node* next;
} Node;

Node* head = NULL;

Node* insert(Node* root, int value) {
    if (value < root->value) {
        Node* next = root;
        root = (Node*)malloc(sizeof(Node));
        root->next = next;
        root->prev = NULL;
        root->value = value;
        return root;
    }
    else {
        Node* node = root;
        while (value >= node->value) {
            if (node->next == NULL) {
                Node* new_node = (Node*)malloc(sizeof(Node));
                new_node->prev = node;
                new_node->next = NULL;
                new_node->value = value;
                node->next = new_node;
                return root;
            }
            else
                node = node->next;
        }
        Node* new_node = (Node*)malloc(sizeof(Node));
        new_node->value = value;
        new_node->prev = node->prev;
        new_node->next = node;

        node->prev->next = new_node;
        node->prev = new_node;

        return root;
    }
}*/

static void clook_merged_requests(struct request_queue *q, struct request *rq,
                 struct request *next)
{
    list_del_init(&next->queuelist);
}

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

static void clook_add_request(struct request_queue *q, struct request *rq)
{
    // Original function appends rq to q
    //~ struct clook_data *nd = q->elevator->elevator_data;
    //~ list_add_tail(&rq->queuelist, &nd->queue);


    // We insert rq at a specific location of q.

    // Added requests are stored and sorted to the right of head:
    // HEAD -> 1 -> 2 -> 3 -> 5-> ...

    // Sort criteria is blk_rq_pos:
    // https://elixir.bootlin.com/linux/v3.19/source/include/linux/blkdev.h#L873

    // Both, &rq->queuelist and &nd->queue return pointers to list_head, which is a node

    // list_head contains previous and next list_head pointers:
    // https://elixir.bootlin.com/linux/v3.19/source/drivers/gpu/drm/nouveau/nvif/list.h#L110
    // - which resembles a doubly-linked list

    // The following function prepends item to node (node->prev = item) and relinks surrounding nodes:
    // list_add_tail(item, node)

    // The following function appends item to node (node->next = item) and relinks surrounding nodes:
    // list_add(item, node)

    // 1. We start at the HEAD (nd->queue) and traverse forward until:
    //   - next points to HEAD, meaning at last node
    //   - next points to a node, whose block position is greater than blk_rq_pos(rq)
    // 2. Append to node

    // In this case, we achieve a sorted doubly linked list (takes O(n) but does the job)

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

static struct request *
clook_former_request(struct request_queue *q, struct request *rq)
{
    struct clook_data *nd = q->elevator->elevator_data;

    if (rq->queuelist.prev == &nd->queue)
        return NULL;
    return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
clook_latter_request(struct request_queue *q, struct request *rq)
{
    struct clook_data *nd = q->elevator->elevator_data;

    if (rq->queuelist.next == &nd->queue)
        return NULL;
    return list_entry(rq->queuelist.next, struct request, queuelist);
}

static int clook_init_queue(struct request_queue *q, struct elevator_type *e)
{
    struct clook_data *nd;
    struct elevator_queue *eq;

    eq = elevator_alloc(q, e);
    if (!eq)
        return -ENOMEM;

    nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
    if (!nd) {
        kobject_put(&eq->kobj);
        return -ENOMEM;
    }
    eq->elevator_data = nd;

    INIT_LIST_HEAD(&nd->queue);

    spin_lock_irq(q->queue_lock);
    q->elevator = eq;
    spin_unlock_irq(q->queue_lock);
    return 0;
}

static void clook_exit_queue(struct elevator_queue *e)
{
    struct clook_data *nd = e->elevator_data;

    BUG_ON(!list_empty(&nd->queue));
    kfree(nd);
}

static struct elevator_type elevator_clook = {
    .ops = {
        .elevator_merge_req_fn      = clook_merged_requests,
        .elevator_dispatch_fn       = clook_dispatch,
        .elevator_add_req_fn        = clook_add_request,
        .elevator_former_req_fn     = clook_former_request,
        .elevator_latter_req_fn     = clook_latter_request,
        .elevator_init_fn       = clook_init_queue,
        .elevator_exit_fn       = clook_exit_queue,
    },
    .elevator_name = "clook",
    .elevator_owner = THIS_MODULE,
};

static int __init clook_init(void)
{
    return elv_register(&elevator_clook);
}

static void __exit clook_exit(void)
{
    elv_unregister(&elevator_clook);
}

module_init(clook_init);
module_exit(clook_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CLOOK IO scheduler");
