# Engine Decision Record (EDR)

## Purpose
Document the evaluation and selection of rendering engines for each supported format.

---

## PDF Engine

### Candidates

| Engine | License | Render | Text Extract | Annotations | Save/Edit | Thread Safety |
|--------|---------|--------|-------------|-------------|-----------|---------------|
| **MuPDF** | AGPL-3.0 / Commercial | Excellent | Yes | Yes (full) | Yes | No (serialize) |
| PDFium | BSD-3-Clause | Excellent | Yes | Partial | Limited | No (serialize) |
| Poppler | GPL-2.0 | Good | Yes | Read-only | No | No |

### Decision: **MuPDF** (primary) + **QPDF** (structural operations)

**Rationale:**
- MuPDF provides the most complete feature set: high-quality rendering, text extraction, full annotation support (create/edit/save), and document modification.
- QPDF (Apache-2.0) complements MuPDF for content-preserving structural operations (page delete/insert/reorder, merge/split) where MuPDF's API is less convenient.
- PDFium was considered but its annotation editing/saving capabilities are limited compared to MuPDF.
- Poppler lacks save/edit capabilities needed for annotations.

**License implications:** Project must be AGPL-3.0 compatible (open-source distribution required, which aligns with project goals). Alternatively, commercial MuPDF license can be obtained.

**Thread safety mitigation:** All MuPDF calls go through a per-document serial dispatcher.

### PoC Validation Checklist
- [x] Render PDF page to bitmap
- [x] Extract text with positions
- [x] Add highlight annotation
- [x] Save PDF with annotations
- [x] Reopen and verify annotations displayed
- [x] Handle corrupted PDF gracefully

---

## DjVu Engine

### Decision: **DjVuLibre**

| Engine | License | Notes |
|--------|---------|-------|
| **DjVuLibre** | GPL-2.0 | Industry standard, used by WinDjView |

**Rationale:**
- DjVuLibre is the only mature, maintained C++ library for DjVu decoding.
- WinDjView (the most popular Windows DjVu viewer) uses DjVuLibre, proving the approach viable.
- Supports: page rendering, text layer extraction, outline/bookmarks, hyperlinks.

### PoC Validation Checklist
- [x] Render DjVu page to bitmap
- [x] Extract outline/TOC
- [x] Extract text layer (when present)
- [x] Search within text layer
- [x] Handle multi-page DjVu documents

---

## CBZ Engine

### Decision: **Custom implementation** (minizip + stb_image/libjpeg-turbo/libpng)

**Rationale:**
- CBZ is simply a ZIP archive containing sequentially-named image files.
- No specialized engine needed — just ZIP extraction and image decoding.
- minizip-ng (zlib license) for ZIP reading.
- Image decoding: stb_image (public domain) for common formats, with libjpeg-turbo and libpng as optional high-performance backends.

### PoC Validation Checklist
- [x] Open ZIP, enumerate and sort image entries
- [x] Decode JPEG/PNG/WebP images
- [x] Implement lazy loading for large archives (1000+ images)
- [x] Continuous scroll with virtualization

---

## EPUB Engine

### Decision: **WebView2** (Microsoft Edge Chromium embedded)

| Approach | License | Pros | Cons |
|----------|---------|------|------|
| **WebView2** | Proprietary (free runtime) | Full HTML5/CSS3/JS, accurate EPUB3 rendering | Requires Edge WebView2 Runtime |
| Readium SDK | GPL-3.0 / Commercial | Purpose-built for EPUB | Complex, dual-licensed, less maintained |
| Custom HTML parser | N/A | No dependencies | Enormous effort, incomplete CSS support |

**Rationale:**
- EPUB is fundamentally HTML/CSS content in a ZIP container.
- WebView2 provides pixel-perfect rendering of complex EPUB3 content with zero custom layout engine work.
- Microsoft states "vast majority" of Windows 10/11 devices have WebView2 Runtime pre-installed.
- Critical: all external URL loading must be blocked (intercept navigation events) to maintain offline-only guarantee.

**Fallback:** If WebView2 Runtime is not available, show user-friendly message with instructions to install it (no auto-download).

### PoC Validation Checklist
- [x] Load EPUB content into WebView2
- [x] Navigate TOC / spine
- [x] Search within content
- [x] Block all external URL requests
- [x] Handle EPUB2 and EPUB3

---

## OCR Engine

### Decision: **Tesseract 5.x**

| Engine | License | Notes |
|--------|---------|-------|
| **Tesseract** | Apache-2.0 | Industry standard, LSTM-based, multi-language |

**Rationale:**
- Tesseract is the most mature open-source OCR engine.
- Apache 2.0 license is maximally permissive.
- Supports 100+ languages; Russian (required) is well-supported.
- LSTM-based recognition (Tesseract 4+/5) provides significantly better accuracy than legacy mode.

**Architecture:**
```
Page Bitmap → Preprocessing → Tesseract → Postprocessing → Text Index
                  │                              │
            - Deskew              - Word/line grouping
            - Denoise             - Paragraph detection
            - Binarize            - Confidence filtering
            - Orientation detect
```

### PoC Validation Checklist
- [x] OCR a scanned page (Russian text)
- [x] OCR a selected region
- [x] Output text with bounding boxes
- [x] Handle rotated pages (auto-orientation detection)

---

## Structural PDF Operations

### Decision: **QPDF**

| Library | License | Purpose |
|---------|---------|---------|
| **QPDF** | Apache-2.0 | Content-preserving PDF transformations |

**Rationale:**
- QPDF specializes in structural PDF operations: page deletion, insertion, reordering, rotation, merging.
- It does not render or extract text — it complements MuPDF.
- Apache 2.0 license is maximally permissive.
- Handles linearization, encryption, and repair of damaged PDFs.

---

## Summary of Dependencies

| Component | Library | License | Version |
|-----------|---------|---------|---------|
| PDF Render/Annotate | MuPDF | AGPL-3.0 | 1.24+ |
| PDF Structure | QPDF | Apache-2.0 | 11.x |
| DjVu | DjVuLibre | GPL-2.0 | 3.5.28+ |
| CBZ/ZIP | minizip-ng | zlib | 4.x |
| Images | stb_image | Public Domain | latest |
| Images (optional) | libjpeg-turbo | BSD-3 | 3.x |
| EPUB Render | WebView2 | Proprietary (free) | latest |
| OCR | Tesseract | Apache-2.0 | 5.x |
| OCR prereq | Leptonica | BSD-2 | 1.84+ |
| Database | SQLite | Public Domain | 3.x |
| JSON | nlohmann/json | MIT | 3.x |

**Overall project license:** AGPL-3.0 (driven by MuPDF and DjVuLibre requirements).
