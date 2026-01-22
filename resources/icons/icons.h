#ifndef ICONS_H
#define ICONS_H

/**
 * Embedded icon resources for GitSardine
 *
 * Icons are embedded as byte arrays for static linking.
 * Usage:
 *   QPixmap pixmap;
 *   pixmap.loadFromData(icon_arrow_circle_315_png, icon_arrow_circle_315_png_len, "PNG");
 *   QIcon icon(pixmap);
 */

// Button icons
#include "arrow_circle_315.h"
#include "cross_button.h"
#include "arrow_skip_270.h"
#include "arrow_skip_090.h"
#include "cross.h"
#include "arrow_curve_180_left.h"
#include "disk_minus.h"

// Navigation icons
#include "navigation_000_button_white.h"
#include "navigation_180_button_white.h"

// Spinner animation frames
#include "spin_1.h"
#include "spin_2.h"
#include "spin_3.h"
#include "spin_4.h"

// Status icons
#include "exclamation_red.h"
#include "drive_download.h"
#include "drive_upload.h"
#include "disk_plus.h"
#include "document.h"

// Tree icons
#include "folder_horizontal.h"
#include "drive.h"

#endif // ICONS_H
