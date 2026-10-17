#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include "lvgl.h"

#define N_DRINKS  4
#define MAX_SUGAR 5

typedef struct { const char *name; int price; int brew_ms; } drink_t;

static const drink_t drinks[N_DRINKS] = {
    {"Espresso",   150, 3000},
    {"Americano",  200, 4000},
    {"Cappuccino", 250, 5000},
    {"Latte",      300, 5500},
};

static const char *stage_msg[] = {"Grinding beans...", "Heating water...", "Brewing..."};

static lv_obj_t *drink_btn[N_DRINKS];
static lv_obj_t *credit_lbl, *name_lbl, *price_lbl, *sugar_lbl;
static lv_obj_t *status_lbl, *brew_bar, *brew_btn;
static int selected = 0, sugar = 2, credit = 0, brewing = 0, brew_stage = -1;

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

static void set_status(const char *s) { lv_label_set_text(status_lbl, s); }

static void refresh(void)
{
    lv_label_set_text_fmt(credit_lbl, "Credit: $%d.%02d", credit / 100, credit % 100);
    for (int i = 0; i < N_DRINKS; i++) {
        if (i == selected) lv_obj_add_state(drink_btn[i], LV_STATE_CHECKED);
        else               lv_obj_remove_state(drink_btn[i], LV_STATE_CHECKED);
    }
    lv_label_set_text(name_lbl, drinks[selected].name);
    lv_label_set_text_fmt(price_lbl, "Price: $%d.%02d",
                          drinks[selected].price / 100, drinks[selected].price % 100);
    lv_label_set_text_fmt(sugar_lbl, "%d / %d", sugar, MAX_SUGAR);
    if (!brewing && credit >= drinks[selected].price)
        lv_obj_remove_state(brew_btn, LV_STATE_DISABLED);
    else
        lv_obj_add_state(brew_btn, LV_STATE_DISABLED);
}

static void drink_cb(lv_event_t *e)
{
    if (brewing) return;
    selected = (int)(intptr_t)lv_event_get_user_data(e);
    set_status(credit >= drinks[selected].price ? "Press BREW" : "Insert more coins");
    refresh();
}

static void coin_cb(lv_event_t *e)
{
    if (brewing) return;
    int amount = (int)(intptr_t)lv_event_get_user_data(e);
    if (amount == 0) { credit = 0; set_status("Credit refunded"); }
    else {
        credit += amount;
        if (credit > 2000) credit = 2000;
        set_status(credit >= drinks[selected].price ? "Press BREW" : "Insert more coins");
    }
    refresh();
}

static void sugar_cb(lv_event_t *e)
{
    if (brewing) return;
    sugar += (int)(intptr_t)lv_event_get_user_data(e);
    if (sugar < 0) sugar = 0;
    if (sugar > MAX_SUGAR) sugar = MAX_SUGAR;
    refresh();
}

static void bar_exec(void *obj, int32_t v)
{
    lv_bar_set_value((lv_obj_t *)obj, v, LV_ANIM_OFF);
    int st = v < 25 ? 0 : (v < 60 ? 1 : 2);
    if (st != brew_stage) { brew_stage = st; set_status(stage_msg[st]); }
}

static void brew_done(lv_anim_t *a)
{
    (void)a;
    brewing = 0;
    set_status("Done! Enjoy your coffee");
    refresh();
}

static void brew_cb(lv_event_t *e)
{
    (void)e;
    if (brewing || credit < drinks[selected].price) return;
    credit -= drinks[selected].price;
    brewing = 1;
    brew_stage = -1;
    lv_bar_set_value(brew_bar, 0, LV_ANIM_OFF);
    refresh();

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, brew_bar);
    lv_anim_set_exec_cb(&a, bar_exec);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, drinks[selected].brew_ms);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_completed_cb(&a, brew_done);
    lv_anim_start(&a);
}

static lv_obj_t *make_label(lv_obj_t *p, int x, int y, const lv_font_t *f,
                            uint32_t color, const char *t)
{
    lv_obj_t *l = lv_label_create(p);
    lv_label_set_text(l, t);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_pos(l, x, y);
    return l;
}

static lv_obj_t *make_btn(lv_obj_t *p, int x, int y, int w, int h, uint32_t bg,
                          const char *text, lv_event_cb_t cb, void *ud)
{
    lv_obj_t *b = lv_button_create(p);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_text_color(b, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_center(l);
    return b;
}

static lv_obj_t *make_box(lv_obj_t *p, int x, int y, int w, int h, uint32_t bg, int radius)
{
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, lv_color_hex(bg), 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static void build_ui(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E1410), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* header */
    lv_obj_t *hdr = make_box(scr, 0, 0, 480, 32, 0x6F4E37, 0);
    make_label(hdr, 10, 6, &lv_font_montserrat_20, 0xFFFFFF, "COFFEE MACHINE");
    credit_lbl = make_label(hdr, 0, 0, &lv_font_montserrat_16, 0xFFE9A8, "");
    lv_obj_align(credit_lbl, LV_ALIGN_RIGHT_MID, -10, 0);

    /* drink selection (2x2) */
    for (int i = 0; i < N_DRINKS; i++) {
        lv_obj_t *b = lv_button_create(scr);
        lv_obj_set_pos(b, 8 + (i % 2) * 120, 40 + (i / 2) * 76);
        lv_obj_set_size(b, 112, 70);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x5A4030), 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0xC8803C), LV_STATE_CHECKED);
        lv_obj_set_style_text_color(b, lv_color_hex(0xFFFFFF), 0);
        lv_obj_add_event_cb(b, drink_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t *n = lv_label_create(b);
        lv_label_set_text(n, drinks[i].name);
        lv_obj_set_style_text_font(n, &lv_font_montserrat_16, 0);
        lv_obj_align(n, LV_ALIGN_CENTER, 0, -10);

        lv_obj_t *p = lv_label_create(b);
        lv_label_set_text_fmt(p, "$%d.%02d", drinks[i].price / 100, drinks[i].price % 100);
        lv_obj_align(p, LV_ALIGN_CENTER, 0, 14);
        drink_btn[i] = b;
    }

    /* coins */
    make_btn(scr, 8,   194, 72, 36, 0x3C6E8F, "+$1",   coin_cb, (void *)(intptr_t)100);
    make_btn(scr, 86,  194, 72, 36, 0x3C6E8F, "+$5",   coin_cb, (void *)(intptr_t)500);
    make_btn(scr, 164, 194, 76, 36, 0x8F3C3C, "Refund", coin_cb, (void *)(intptr_t)0);

    /* right panel */
    lv_obj_t *pn = make_box(scr, 248, 40, 224, 190, 0x3B2A20, 8);
    name_lbl  = make_label(pn, 10, 8,  &lv_font_montserrat_20, 0xFFFFFF, "");
    price_lbl = make_label(pn, 10, 36, &lv_font_montserrat_14, 0xFFE9A8, "");

    make_label(pn, 10, 68, &lv_font_montserrat_14, 0xF2E6D8, "Sugar");
    make_btn(pn, 100, 58, 36, 32, 0x5A4030, LV_SYMBOL_MINUS, sugar_cb, (void *)(intptr_t)-1);
    sugar_lbl = make_label(pn, 140, 68, &lv_font_montserrat_14, 0xFFFFFF, "");
    lv_obj_set_width(sugar_lbl, 38);
    lv_obj_set_style_text_align(sugar_lbl, LV_TEXT_ALIGN_CENTER, 0);
    make_btn(pn, 180, 58, 36, 32, 0x5A4030, LV_SYMBOL_PLUS, sugar_cb, (void *)(intptr_t)1);

    brew_bar = lv_bar_create(pn);
    lv_obj_set_pos(brew_bar, 10, 112);
    lv_obj_set_size(brew_bar, 204, 14);
    lv_obj_set_style_bg_color(brew_bar, lv_color_hex(0x1E1410), 0);
    lv_obj_set_style_bg_color(brew_bar, lv_color_hex(0xC8803C), LV_PART_INDICATOR);

    brew_btn = make_btn(pn, 10, 136, 204, 44, 0x2E8B57, "BREW", brew_cb, NULL);
    lv_obj_set_style_bg_color(brew_btn, lv_color_hex(0x555555), LV_STATE_DISABLED);

    /* status line */
    status_lbl = make_label(scr, 8, 242, &lv_font_montserrat_14, 0xF2E6D8, "");
    set_status("Insert coins and pick a drink");
    refresh();
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

    build_ui();

    for (;;) {
        lv_timer_handler();
        usleep(5000);
    }
}
