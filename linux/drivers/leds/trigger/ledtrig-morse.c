#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/sched/loadavg.h>
#include <linux/leds.h>
#include <linux/reboot.h>
#include <linux/ioctl.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include "../leds.h"


/*
 -------------------------------------------------------------------------------
 Structures
 -------------------------------------------------------------------------------
*/

typedef struct
{
    int status, dignity, ego;
} msg_arg_t;

struct morse_trig_data {
    char* message;
    unsigned int indexL; // Letter index
    unsigned int indexM; // Letter part index
    unsigned int delayM; // Every letter part is followed a delay. This indicates whether to wait or proceed to the next part.
    unsigned int speed; // 1 - 10000
    unsigned int mode; // 0 - one-off; 1 - repeat
    struct timer_list timer;
};


/*
 -------------------------------------------------------------------------------
 Defines
 -------------------------------------------------------------------------------
*/

#define FIRST_MINOR 0
#define MINOR_CNT 1

#define MSG_GET_VARIABLES _IOR('q', 1, msg_arg_t *)
#define MSG_CLR_VARIABLES _IO('q', 2)
#define MSG_SET_VARIABLES _IOW('q', 3, msg_arg_t *)


/*
 -------------------------------------------------------------------------------
 Constants
 -------------------------------------------------------------------------------
*/

const char* LETTER_TO_MORSE[] = {
    ".-", //A
    "-...", //B
    "-.-.", //C
    "-..", //D
    ".", //E
    "..-.", //F
    "--.", //G
    "....", //H
    "..", //I
    ".---", //J
    "-.-", //K
    ".-..", //L
    "--", //M
    "-.", //N
    "---", //O
    ".--.", //P
    "--.-", //Q
    ".-.", //R
    "...", //S
    "-", //T
    "..-", //U
    "...-", //V
    ".--", //W
    "-..-", //X
    "-.--", //Y
    "--.." //Z
};

const char* DIGIT_TO_MORSE[] = {
    "-----", // 0
    ".----", // 1
    "..---", // 2
    "...--", // 3
    "....-", // 4
    ".....", // 5
    "-....", // 6
    "--...", // 7
    "---..", // 8
    "----." // 9
};

const int DOT_LENGTH = 1;
const int DASH_LENGTH = 3;

const int MESSAGE_DELAY = 20;
const int WORD_DELAY = 7;
const int LETTER_DELAY = 3;
const int PART_DELAY = 1;

const int INVALID_CHAR_DELAY = 3;

const unsigned int DEFAULT_SPEED = 50;
const unsigned int DEFAULT_MODE = 1;

const char* DEFAULT_MESSAGE = "Linux Operating Systems Group 1";


/*
 -------------------------------------------------------------------------------
 Global Variables
 -------------------------------------------------------------------------------
*/

static int panic_morses;
static dev_t dev;
static struct cdev c_dev;
static struct class *cl;


/*
 -------------------------------------------------------------------------------
 Prototypes
 -------------------------------------------------------------------------------
*/

static void morse_trig_deactivate(struct led_classdev *led_cdev);


/*
 -------------------------------------------------------------------------------
 Helper Functions
 -------------------------------------------------------------------------------
*/

const char* char_to_morse(char c) {
    if (c >= 0x41 && c <= 0x5A) {
        return LETTER_TO_MORSE[c - 0x41];
    }
    else if (c >= 0x61 && c <= 0x7A) {
        return LETTER_TO_MORSE[c - 0x61];
    }
    else if (c >= 0x30 && c <= 0x39) {
        return DIGIT_TO_MORSE[c - 0x30];
    }
    else {
        return NULL;
    }
}

static void store_message(struct morse_trig_data *morse_data, const char *buf, size_t size) {

    if (morse_data->message)
        kfree(morse_data->message);

    morse_data->message = kstrndup(buf, size, GFP_KERNEL);
    morse_data->indexL = 0;
    morse_data->indexM = 0;
    morse_data->delayM = 0;
}


/*
 -------------------------------------------------------------------------------
 Character Device
 -------------------------------------------------------------------------------
*/

static int my_open(struct inode *i, struct file *f)
{
    printk(KERN_ALERT "CS444 MSG driver open\r\n");
    return 0;
}
static int my_close(struct inode *i, struct file *f)
{
    printk(KERN_ALERT "CS444 MSG driver close\r\n");
    return 0;
}

static ssize_t msg_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    printk(KERN_ALERT "CS444 MSG driver read\r\n");
    snprintf(buf, size, "Hey there, I'm a msg!\r\n");
    return strlen(buf);
}

static ssize_t msg_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
    char lcl_buf[64];

    memset(lcl_buf, 0, sizeof(lcl_buf));

    if (copy_from_user(lcl_buf, buf, min(size, sizeof(lcl_buf))))
        {
            return -EACCES;
        }

    printk(KERN_ALERT "CS444 MSG driver write %ld bytes: %s\r\n", size, lcl_buf);

    return size;
}

static struct file_operations msg_fops =
{
    .owner = THIS_MODULE,
    .open = my_open,
    .read = msg_read,
    .write = msg_write,
    .release = my_close
};

/*
 -------------------------------------------------------------------------------
 LED Morse Trigger
 -------------------------------------------------------------------------------
*/

static void led_morse_function(unsigned long data)
{
    struct led_classdev *led_cdev = (struct led_classdev *) data;
    struct morse_trig_data *morse_data = led_cdev->trigger_data;
    unsigned long brightness = LED_OFF;
    unsigned long delay = 0;
    char letter;

    if (unlikely(panic_morses)) {
        led_set_brightness_nosleep(led_cdev, LED_OFF);
        return;
    }

    if (test_and_clear_bit(LED_BLINK_BRIGHTNESS_CHANGE, &led_cdev->work_flags))
        led_cdev->blink_brightness = led_cdev->new_blink_brightness;

    // Assuming morse_data->message != NULL and is a null-terminated strings
    letter = morse_data->message[morse_data->indexL];

    // Start sentence for debugging
    if (morse_data->indexL == 0 && morse_data->indexM == 0) {
        //printk(KERN_ALERT "Morse: ");
        //printk(KERN_ALERT "MESSAGE BEGIN ...\n");
    }

    if (letter == '\0' || letter == '\n') {
        // If finished processing last letter of the message, either:

        // "deactivate"
        if (morse_data->mode == 0) {
            //~ led_set_brightness_nosleep(led_cdev, LED_OFF);
            //~ morse_trig_deactivate(led_cdev);
            //~ return;
            delay = MESSAGE_DELAY;
        }
        else {
            // set delay to word delay and restart
            delay = MESSAGE_DELAY;
            morse_data->indexL = 0;
            morse_data->indexM = 0;
            morse_data->delayM = 0;
        }
        //printk(KERN_ALERT "... MESSAGE END\n");

    }
    else if (letter == ' ') {
        // If finished processing last letter of the word, set delay to word delay and increment letter index
        delay = WORD_DELAY;
        ++(morse_data->indexL);
        morse_data->indexM = 0;
        morse_data->delayM = 0;
        //printk(KERN_CONT "/ ");
    }
    else if (morse_data->delayM != 0) {
        delay = morse_data->delayM;
        morse_data->delayM = 0;
    }
    else {
        // Process letter part
        const char* parts = char_to_morse(letter);

        if (parts == NULL) {
            // If invalid parts, advance to next letter
            printk(KERN_ALERT "Unrecognized Morse code character, \"%c\".\n", letter);

            ++(morse_data->indexL);
            morse_data->indexM = 0;
            morse_data->delayM = 0;
            delay = INVALID_CHAR_DELAY;
        }
        else {
            // If valid parts, set delay to current part
            char part = parts[morse_data->indexM];

            if (part == '-') {
                delay = DASH_LENGTH;
                brightness = led_cdev->blink_brightness;
                ++(morse_data->indexM);
                morse_data->delayM = PART_DELAY;
                //printk(KERN_CONT "-");
            }
            else if (part == '.') {
                delay = DOT_LENGTH;
                brightness = led_cdev->blink_brightness;
                ++(morse_data->indexM);
                morse_data->delayM = PART_DELAY;
                //printk(KERN_CONT ".");
            }
            else {
                // Advance to next word/letter if finished processing the current letter.
                ++(morse_data->indexL);
                morse_data->indexM = 0;
                morse_data->delayM = WORD_DELAY;
                //printk(KERN_CONT " ");
            }
        }
    }

    led_set_brightness_nosleep(led_cdev, brightness);
    mod_timer(&morse_data->timer, jiffies + msecs_to_jiffies(delay * morse_data->speed));
}

static ssize_t led_speed_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;

    return sprintf(buf, "%d\n", morse_data->speed);
}

static ssize_t led_speed_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;
    unsigned long s;
    int ret;

    ret = kstrtoul(buf, 0, &s);
    if (ret)
        return ret;

    if (s < 1)
        s = 1;
    else if (s > 10000)
        s = 10000;

    morse_data->speed = s;

    return size;
}

static ssize_t led_message_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;

    return sprintf(buf, "%s\n", morse_data->message);
}

static ssize_t led_message_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;

    store_message(morse_data, buf, size);

    return size;
}

static ssize_t led_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;

    if (morse_data->mode == 0) {
        return sprintf(buf, "%d: one-off\n", morse_data->mode);
    }
    else {
        return sprintf(buf, "%d: repeat\n", morse_data->mode);
    }
}

static ssize_t led_mode_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;
    unsigned long s;
    int ret;

    ret = kstrtoul(buf, 0, &s);
    if (ret) return ret;

    if (s == 0)
        morse_data->mode = 0;
    else
        morse_data->mode = 1;

    return size;
}

static DEVICE_ATTR(speed, 0644, led_speed_show, led_speed_store);
static DEVICE_ATTR(message, 0644, led_message_show, led_message_store);
static DEVICE_ATTR(mode, 0644, led_mode_show, led_mode_store);

static void morse_trig_activate(struct led_classdev *led_cdev)
{
    struct morse_trig_data *morse_data;
    int rc;

    morse_data = kzalloc(sizeof(*morse_data), GFP_KERNEL);
    if (!morse_data)
        return;

    led_cdev->trigger_data = morse_data;

    rc = device_create_file(led_cdev->dev, &dev_attr_speed);
    if (rc) {
        kfree(led_cdev->trigger_data);
        return;
    }

    rc = device_create_file(led_cdev->dev, &dev_attr_message);
    if (rc) {
        kfree(led_cdev->trigger_data);
        return;
    }

    rc = device_create_file(led_cdev->dev, &dev_attr_mode);
    if (rc) {
        kfree(led_cdev->trigger_data);
        return;
    }

    setup_timer(&morse_data->timer, led_morse_function, (unsigned long) led_cdev);
    morse_data->message = NULL;
    morse_data->indexL = 0;
    morse_data->indexM = 0;
    morse_data->delayM = 0;
    morse_data->speed = DEFAULT_SPEED;
    morse_data->mode = DEFAULT_MODE;
    store_message(morse_data, DEFAULT_MESSAGE, 256);

    printk(KERN_ALERT "Message: %s, Speed: %d, Mode: %d\n", morse_data->message, morse_data->speed, morse_data->mode);

    if (!led_cdev->blink_brightness)
        led_cdev->blink_brightness = led_cdev->max_brightness;

    led_morse_function(morse_data->timer.data);
    set_bit(LED_BLINK_SW, &led_cdev->work_flags);
    led_cdev->activated = true;
}

static void morse_trig_deactivate(struct led_classdev *led_cdev)
{
    struct morse_trig_data *morse_data = led_cdev->trigger_data;

    if (led_cdev->activated) {
        del_timer_sync(&morse_data->timer);

        device_remove_file(led_cdev->dev, &dev_attr_speed);
        device_remove_file(led_cdev->dev, &dev_attr_message);
        device_remove_file(led_cdev->dev, &dev_attr_mode);

        if (morse_data->message) {
            kfree(morse_data->message);
            morse_data->message = NULL;
        }
        kfree(morse_data);

        clear_bit(LED_BLINK_SW, &led_cdev->work_flags);
        led_cdev->activated = false;
    }
}

static struct led_trigger morse_led_trigger = {
    .name       = "morse",
    .activate   = morse_trig_activate,
    .deactivate = morse_trig_deactivate,
};

static int morse_reboot_notifier(struct notifier_block *nb, unsigned long code, void *unused)
{
    led_trigger_unregister(&morse_led_trigger);
    return NOTIFY_DONE;
}

static int morse_panic_notifier(struct notifier_block *nb, unsigned long code, void *unused)
{
    panic_morses = 1;
    return NOTIFY_DONE;
}

static struct notifier_block morse_reboot_nb = {
    .notifier_call = morse_reboot_notifier,
};

static struct notifier_block morse_panic_nb = {
    .notifier_call = morse_panic_notifier,
};

static int __init morse_trig_init(void)
{
    int ret;
    struct device *dev_ret;

    // Register trigger
    int rc = led_trigger_register(&morse_led_trigger);

    if (!rc) {
        atomic_notifier_chain_register(&panic_notifier_list, &morse_panic_nb);
        register_reboot_notifier(&morse_reboot_nb);
    }

    // Allocate the device
    if ((ret = alloc_chrdev_region(&dev, FIRST_MINOR, MINOR_CNT, "msg")) < 0)
    {
        return ret;
    }

    cdev_init(&c_dev, &msg_fops);

    if ((ret = cdev_add(&c_dev, dev, MINOR_CNT)) < 0)
    {
        return ret;
    }

    // Allocate the /dev device (/dev/msg)
    if (IS_ERR(cl = class_create(THIS_MODULE, "char")))
    {
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(cl);
    }
    if (IS_ERR(dev_ret = device_create(cl, NULL, dev, NULL, "msg")))
    {
        class_destroy(cl);
        cdev_del(&c_dev);
        unregister_chrdev_region(dev, MINOR_CNT);
        return PTR_ERR(dev_ret);
    }

    printk(KERN_ALERT "CS444 MSG Driver has been loaded!\r\n");

    return rc;
}

static void __exit morse_trig_exit(void)
{
    // Unregister trigger
    unregister_reboot_notifier(&morse_reboot_nb);
    atomic_notifier_chain_unregister(&panic_notifier_list, &morse_panic_nb);
    led_trigger_unregister(&morse_led_trigger);

    // Exit character device
    device_destroy(cl, dev);
    class_destroy(cl);
    cdev_del(&c_dev);
    unregister_chrdev_region(dev, MINOR_CNT);
}

module_init(morse_trig_init);
module_exit(morse_trig_exit);

MODULE_AUTHOR("CS444 Group 1");
MODULE_DESCRIPTION("Morse LED trigger");
MODULE_LICENSE("GPL");
