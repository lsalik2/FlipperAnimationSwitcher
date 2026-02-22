#include "fas_list_view.h"

#include <furi.h>
#include <gui/canvas.h>
#include <gui/elements.h>
#include <input/input.h>
#include <string.h>
#include <stdio.h>

/* ── Layout constants ─────────────────────────────────────────────────── */
#define VISIBLE_ROWS   4
#define ROW_HEIGHT    14
#define LIST_Y_START   2
#define CHECKBOX_W    10
#define CHECKBOX_H    10