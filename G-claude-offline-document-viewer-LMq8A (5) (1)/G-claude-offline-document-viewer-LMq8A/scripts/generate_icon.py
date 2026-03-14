#!/usr/bin/env python3
"""Generate DocVision application icon.

Creates a multi-size .ico file with a document viewer icon:
- Open book/document with a magnifying glass accent
- Blue gradient theme, professional look
"""

from PIL import Image, ImageDraw, ImageFont
import math
import os

def draw_icon(size):
    """Draw the DocVision icon at the given size."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    s = size / 256.0  # scale factor relative to 256px base

    # Background: rounded rectangle with blue gradient effect
    # Using a solid rich blue since PIL doesn't do gradients natively
    bg_color = (41, 98, 180, 255)       # Primary blue
    bg_dark = (28, 69, 135, 255)        # Darker blue for depth
    accent = (66, 165, 245, 255)        # Light blue accent
    white = (255, 255, 255, 255)
    shadow = (20, 50, 100, 180)

    # Draw rounded background
    margin = int(12 * s)
    radius = int(40 * s)

    # Shadow
    shadow_off = int(4 * s)
    draw.rounded_rectangle(
        [margin + shadow_off, margin + shadow_off, size - margin + shadow_off, size - margin + shadow_off],
        radius=radius, fill=(0, 0, 0, 60)
    )

    # Main background
    draw.rounded_rectangle(
        [margin, margin, size - margin, size - margin],
        radius=radius, fill=bg_color
    )

    # Darker bottom half for depth
    half_y = size // 2
    draw.rounded_rectangle(
        [margin, half_y, size - margin, size - margin],
        radius=radius, fill=bg_dark
    )
    # Re-draw top portion to fix overlap
    draw.rectangle(
        [margin + radius, half_y, size - margin - radius, half_y + int(20 * s)],
        fill=bg_color
    )

    # --- Document page (white, slightly rotated look) ---
    doc_left = int(55 * s)
    doc_top = int(45 * s)
    doc_right = int(165 * s)
    doc_bottom = int(210 * s)

    # Document shadow
    draw.rounded_rectangle(
        [doc_left + int(3*s), doc_top + int(3*s), doc_right + int(3*s), doc_bottom + int(3*s)],
        radius=int(8*s), fill=(0, 0, 0, 50)
    )

    # Document body
    draw.rounded_rectangle(
        [doc_left, doc_top, doc_right, doc_bottom],
        radius=int(8*s), fill=white
    )

    # Dog-ear fold on top-right
    fold_size = int(22 * s)
    fold_points = [
        (doc_right - fold_size, doc_top),
        (doc_right, doc_top + fold_size),
        (doc_right, doc_top + int(8*s)),
        (doc_right - int(8*s), doc_top),
    ]
    # White triangle to clip corner
    draw.polygon([
        (doc_right - fold_size, doc_top),
        (doc_right, doc_top),
        (doc_right, doc_top + fold_size),
    ], fill=bg_color)

    # Fold triangle
    draw.polygon([
        (doc_right - fold_size, doc_top),
        (doc_right, doc_top + fold_size),
        (doc_right - fold_size, doc_top + fold_size),
    ], fill=(220, 225, 235, 255))

    # Text lines on document
    line_color = (180, 195, 215, 255)
    line_dark = (100, 130, 170, 255)
    line_y_start = doc_top + int(35 * s)
    line_left = doc_left + int(15 * s)
    line_right_max = doc_right - int(15 * s)
    line_h = max(int(4 * s), 2)
    line_gap = int(14 * s)

    # Title line (wider, darker)
    draw.rounded_rectangle(
        [line_left, line_y_start, line_right_max - int(10*s), line_y_start + line_h + max(int(2*s), 1)],
        radius=max(int(2*s), 1), fill=line_dark
    )

    # Body lines
    line_widths = [1.0, 0.85, 0.95, 0.7, 0.9, 0.6, 0.8]
    for i, w in enumerate(line_widths):
        y = line_y_start + int(18 * s) + i * line_gap
        if y + line_h > doc_bottom - int(15 * s):
            break
        right = line_left + int((line_right_max - line_left) * w)
        draw.rounded_rectangle(
            [line_left, y, right, y + line_h],
            radius=max(int(2*s), 1), fill=line_color
        )

    # --- Magnifying glass (bottom-right, overlapping document) ---
    glass_cx = int(185 * s)
    glass_cy = int(175 * s)
    glass_r = int(35 * s)

    # Handle
    handle_angle = math.radians(45)
    hx1 = glass_cx + int((glass_r - 2*s) * math.cos(handle_angle))
    hy1 = glass_cy + int((glass_r - 2*s) * math.sin(handle_angle))
    hx2 = glass_cx + int((glass_r + 30*s) * math.cos(handle_angle))
    hy2 = glass_cy + int((glass_r + 30*s) * math.sin(handle_angle))
    handle_w = max(int(10 * s), 3)

    # Handle shadow
    draw.line([(hx1+int(2*s), hy1+int(2*s)), (hx2+int(2*s), hy2+int(2*s))],
              fill=(0, 0, 0, 40), width=handle_w + max(int(2*s), 1))

    # Handle body
    draw.line([(hx1, hy1), (hx2, hy2)], fill=(200, 160, 60, 255), width=handle_w)
    draw.line([(hx1, hy1), (hx2, hy2)], fill=(230, 190, 80, 255), width=max(handle_w - int(3*s), 2))

    # Glass circle shadow
    draw.ellipse(
        [glass_cx - glass_r + int(2*s), glass_cy - glass_r + int(2*s),
         glass_cx + glass_r + int(2*s), glass_cy + glass_r + int(2*s)],
        fill=(0, 0, 0, 40)
    )

    # Glass rim
    rim_w = max(int(5 * s), 2)
    draw.ellipse(
        [glass_cx - glass_r, glass_cy - glass_r, glass_cx + glass_r, glass_cy + glass_r],
        fill=(200, 215, 240, 230), outline=(70, 130, 200, 255), width=rim_w
    )

    # Glass inner (lighter, like glass)
    inner_r = glass_r - rim_w
    draw.ellipse(
        [glass_cx - inner_r, glass_cy - inner_r, glass_cx + inner_r, glass_cy + inner_r],
        fill=(220, 235, 255, 200)
    )

    # Glare highlight on glass
    glare_r = max(int(8 * s), 2)
    glare_cx = glass_cx - int(12 * s)
    glare_cy = glass_cy - int(12 * s)
    draw.ellipse(
        [glare_cx - glare_r, glare_cy - glare_r, glare_cx + glare_r, glare_cy + glare_r],
        fill=(255, 255, 255, 120)
    )

    # --- "DV" text watermark (subtle, in the blue area at top) ---
    # Only render on larger sizes
    if size >= 64:
        try:
            font_size = int(28 * s)
            font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", font_size)
        except (OSError, IOError):
            font = ImageFont.load_default()

        text_color = (255, 255, 255, 60)
        bbox = draw.textbbox((0, 0), "DV", font=font)
        tw = bbox[2] - bbox[0]
        tx = int(35 * s)
        ty = int(size - 45 * s)
        draw.text((tx, ty), "DV", fill=text_color, font=font)

    return img


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    icons_dir = os.path.join(script_dir, '..', 'resources', 'icons')
    os.makedirs(icons_dir, exist_ok=True)

    # Generate all standard ICO sizes
    sizes = [16, 24, 32, 48, 64, 128, 256]
    images = []

    for sz in sizes:
        img = draw_icon(sz)
        images.append(img)
        # Also save individual PNGs for the largest sizes
        if sz >= 128:
            png_path = os.path.join(icons_dir, f'docvision_{sz}.png')
            img.save(png_path)
            print(f'  Saved {png_path}')

    # Save .ico with all sizes
    ico_path = os.path.join(icons_dir, 'docvision.ico')
    images[0].save(
        ico_path,
        format='ICO',
        sizes=[(img.size[0], img.size[1]) for img in images],
        append_images=images[1:]
    )
    print(f'  Saved {ico_path}')

    print(f'\nIcon generated with sizes: {sizes}')


if __name__ == '__main__':
    main()
