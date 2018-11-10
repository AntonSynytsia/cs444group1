/*
 * LED Heartbeat Trigger
 *
 * Copyright (C) 2006 Atsushi Nemoto <anemo@mba.ocn.ne.jp>
 *
 * Based on Richard Purdie's ledtrig-timer.c and some arch's
 * CONFIG_HEARTBEAT code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/sched/loadavg.h>
#include <linux/leds.h>
#include <linux/reboot.h>
#include "../leds.h"


static int panic_morses;

struct morse_trig_data {
    char* message;
    unsigned int indexL; // Letter index
    unsigned int indexM; // Letter part index
    unsigned int delayM; // Every letter part is followed a delay. This indicates whether to wait or proceed to the next part.
    unsigned int invert;
    struct timer_list timer;
};

const char* CHAR_TO_MORSE[] = {
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

const int DOT_LENGTH = 1;
const int DASH_LENGTH = 3;

const int MESSAGE_DELAY = 20;
const int WORD_DELAY = 7;
const int LETTER_DELAY = 3;
const int PART_DELAY = 1;

const int DELAY_MAGNITUDE = 50;


const char* char_to_morse(char c) {
    if (c >= 0x41 && c <= 0x5A) {
        return CHAR_TO_MORSE[c - 0x41];
    }
    else if (c >= 0x61 && c <= 0x7A) {
        return CHAR_TO_MORSE[c - 0x61];
    }
    else {
        return NULL;
    }
}

static void led_morse_function(unsigned long data)
{
    struct led_classdev *led_cdev = (struct led_classdev *) data;
    struct morse_trig_data *morse_data = led_cdev->trigger_data;
    unsigned long brightness = LED_OFF;
    unsigned long delay = 0;
    char letter;

    /*if (unlikely(panic_morses)) {
        led_set_brightness_nosleep(led_cdev, LED_OFF);
        return;
    }

    if (test_and_clear_bit(LED_BLINK_BRIGHTNESS_CHANGE, &led_cdev->work_flags))
        led_cdev->blink_brightness = led_cdev->new_blink_brightness;*/

    // Assuming morse_data->message != NULL and is a null-terminated strings
    letter = morse_data->message[morse_data->indexL];

    // Start sentence for debugging
    if (morse_data->indexL == 0 && morse_data->indexM == 0) {
        printk(KERN_ALERT "Morse: ");
    }

    if (letter == '\0') {
        // If finished processing last letter of the message, set delay to word delay and restart
        delay = MESSAGE_DELAY;
        morse_data->indexL = 0;
        morse_data->indexM = 0;
        morse_data->delayM = 0;
    }
    else if (letter == ' ') {
        // If finished processing last letter of the word, set delay to word delay and increment letter index
        delay = WORD_DELAY;
        ++(morse_data->indexL);
        morse_data->indexM = 0;
        morse_data->delayM = 0;
        printk(KERN_CONT "/ ");
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
            printk(KERN_ALERT "Cannot find morse code for character, \"%c\".\n", letter);

            ++(morse_data->indexL);
            morse_data->indexM = 0;
            morse_data->delayM = 0;
        }
        else {
            // If valid parts, set delay to current part
            char part = parts[morse_data->indexM];

            if (part == '-') {
                delay = DASH_LENGTH;
                brightness = led_cdev->blink_brightness;
                ++(morse_data->indexM);
                morse_data->delayM = PART_DELAY;
                printk(KERN_CONT "-");
            }
            else if (part == '.') {
                delay = DOT_LENGTH;
                brightness = led_cdev->blink_brightness;
                ++(morse_data->indexM);
                morse_data->delayM = PART_DELAY;
                printk(KERN_CONT ".");
            }
            else {
                // Advance to next word/letter if finished processing the current letter.
                ++(morse_data->indexL);
                morse_data->indexM = 0;
                morse_data->delayM = WORD_DELAY;
                printk(KERN_CONT " ");
            }
        }
    }

    led_set_brightness_nosleep(led_cdev, brightness);
    mod_timer(&morse_data->timer, jiffies + msecs_to_jiffies(delay * DELAY_MAGNITUDE));
}

static ssize_t led_invert_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;

    return sprintf(buf, "%u\n", morse_data->invert);
}

static ssize_t led_invert_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct morse_trig_data *morse_data = led_cdev->trigger_data;
    unsigned long state;
    int ret;

    ret = kstrtoul(buf, 0, &state);
    if (ret)
        return ret;

    morse_data->invert = !!state;

    return size;
}

static DEVICE_ATTR(invert, 0644, led_invert_show, led_invert_store);

static void morse_trig_activate(struct led_classdev *led_cdev)
{
    struct morse_trig_data *morse_data;
    int rc;

    morse_data = kzalloc(sizeof(*morse_data), GFP_KERNEL);
    if (!morse_data)
        return;

    led_cdev->trigger_data = morse_data;
    rc = device_create_file(led_cdev->dev, &dev_attr_invert);
    if (rc) {
        kfree(led_cdev->trigger_data);
        return;
    }

    setup_timer(&morse_data->timer, led_morse_function, (unsigned long) led_cdev);
    morse_data->message = "Linux operating systems";
    morse_data->indexL = 0;
    morse_data->indexM = 0;
    morse_data->delayM = 0;

    printk(KERN_ALERT "%s\n", morse_data->message);

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
        device_remove_file(led_cdev->dev, &dev_attr_invert);
        kfree(morse_data);
        clear_bit(LED_BLINK_SW, &led_cdev->work_flags);
        led_cdev->activated = false;
    }
}

static struct led_trigger morse_led_trigger = {
    .name     = "morse",
    .activate = morse_trig_activate,
    .deactivate = morse_trig_deactivate,
};

static int morse_reboot_notifier(struct notifier_block *nb,
                     unsigned long code, void *unused)
{
    led_trigger_unregister(&morse_led_trigger);
    return NOTIFY_DONE;
}

static int morse_panic_notifier(struct notifier_block *nb,
                     unsigned long code, void *unused)
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
    int rc = led_trigger_register(&morse_led_trigger);

    if (!rc) {
        atomic_notifier_chain_register(&panic_notifier_list,
                           &morse_panic_nb);
        register_reboot_notifier(&morse_reboot_nb);
    }
    return rc;
}

static void __exit morse_trig_exit(void)
{
    unregister_reboot_notifier(&morse_reboot_nb);
    atomic_notifier_chain_unregister(&panic_notifier_list,
                     &morse_panic_nb);
    led_trigger_unregister(&morse_led_trigger);
}

module_init(morse_trig_init);
module_exit(morse_trig_exit);

MODULE_AUTHOR("CS444 Group 1");
MODULE_DESCRIPTION("Morse LED trigger");
MODULE_LICENSE("GPL");
