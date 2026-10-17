#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include "lvgl.h"

static uint32_t tick_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint32_t)(t.tv_sec * 1000 + t.tv_nsec / 1000000);
}

/* first /dev/input/eventN that reports REL_X (a mouse) */
static int find_mouse(char *path, size_t len)
{
    for (int i = 0; i < 16; i++) {
        unsigned char rel[REL_MAX / 8 + 1] = {0};
        snprintf(path, len, "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        int ok = ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel)), rel) >= 0 &&
                 (rel[REL_X / 8] & (1 << (REL_X % 8)));
        close(fd);
        if (ok) return 1;
    }
    return 0;
}

static void btn_cb(lv_event_t *e)
{
    static int n;
    lv_label_set_text_fmt((lv_obj_t *)lv_event_get_user_data(e), "Clicks: %d", ++n);
}

int main(void)
{
    lv_init();
    lv_tick_set_cb(tick_ms);

    lv_display_t *disp = lv_linux_fbdev_create();
    if (!disp) return 1;
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    char path[32];
    if (find_mouse(path, sizeof(path))) {
        lv_indev_t *mouse = lv_evdev_create(LV_INDEV_TYPE_POINTER, path);
        lv_obj_t *cur = lv_obj_create(lv_layer_sys());
        lv_obj_set_size(cur, 10, 10);
        lv_obj_set_style_bg_color(cur, lv_color_white(), 0);
        lv_obj_set_style_border_color(cur, lv_color_black(), 0);
        lv_obj_set_style_border_width(cur, 1, 0);
        lv_obj_set_style_radius(cur, 5, 0);
        lv_indev_set_cursor(mouse, cur);
    }

    lv_obj_t *scr = lv_screen_active();
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "Clicks: 0");
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(btn, btn_cb, LV_EVENT_CLICKED, lbl);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "LicheePi Nano + LVGL 9");

    for (;;) {
        lv_timer_handler();
        usleep(5000);
    }
}
