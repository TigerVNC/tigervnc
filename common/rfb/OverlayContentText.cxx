// -=- OverlayContentText.cxx
//
// Renders text into a pixel buffer for use as watermark overlay content.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>
#include <cstring>
#include <vector>

#include <core/LogWriter.h>

#include <rfb/OverlayContentText.h>

#include <pixman.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <fontconfig/fontconfig.h>

using namespace rfb;

static core::LogWriter vlog("OverlayContentText");

OverlayContentText::OverlayContentText(const std::string& text, int fontSize,
                                       const std::string& font)
    : _buffer(nullptr) {
  _width = _height = 0;
  _buffer = generateTextBuffer(text, fontSize, &_width, &_height, font);
}

OverlayContentText::~OverlayContentText() { delete[] _buffer; }

// Return a path for a usable font file for text overlay
static std::string findFontFile(const std::string& fontPattern = "") {
  if (!FcInit()) {
    return "";
  }

  FcPattern* pattern =
      FcNameParse(reinterpret_cast<const FcChar8*>(fontPattern.c_str()));
  if (!pattern) {
    return "";
  }

  FcConfigSubstitute(nullptr, pattern, FcMatchPattern);
  FcDefaultSubstitute(pattern);

  FcResult result;
  FcPattern* font = FcFontMatch(nullptr, pattern, &result);

  std::string fontPath;
  if (font) {
    FcChar8* file = nullptr;

    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
      fontPath = reinterpret_cast<char*>(file);
    }
    FcPatternDestroy(font);
  }

  FcPatternDestroy(pattern);

  return fontPath;
}

// Initializes FreeType and loads the watermark font, reloading it whenever
// Is able to find new font during runtime config changes.
static FT_Face getOverlayFont(const std::string& font) {
  static FT_Library library = nullptr;
  static FT_Face face = nullptr;
  static std::string loadedFont;
  static bool loadedFontValid = false;

  if (loadedFontValid && (font == loadedFont))
    return face;

  if (!library && (FT_Init_FreeType(&library) != 0)) {
    vlog.error("Failed to initialize FreeType");
    return nullptr;
  }

  const std::string fontFile = findFontFile(font);
  if (fontFile.empty()) {
    vlog.error("No usable font found for overlay text");
    return nullptr;
  }

  if (face)
    FT_Done_Face(face);

  if (FT_New_Face(library, fontFile.c_str(), 0, &face) != 0) {
    vlog.error("Failed to load font %s for overlay text", fontFile.c_str());
    face = nullptr;
    loadedFontValid = false;
    return nullptr;
  }

  loadedFont = font;
  loadedFontValid = true;
  return face;
}

// Renders text at the given pixel font size into a freshly allocated
// ARGB32 buffer sized exactly to fit the rendered glyphs. Returns nullptr
// if text is empty or no usable font is found.

// TODO: Rewrite/Comment
uint8_t* OverlayContentText::generateTextBuffer(const std::string& text,
                                                int size, int* outWidth,
                                                int* outHeight,
                                                const std::string& font) {
  *outWidth = *outHeight = 0;

  if (text.empty())
    return nullptr;

  FT_Face face = getOverlayFont(font);
  if (!face)
    return nullptr;

  FT_UInt pixelSize = static_cast<FT_UInt>(size);
  FT_Set_Pixel_Sizes(face, 0, pixelSize);

  // First pass: measure the extents so we can allocate a tightly fitting
  // buffer before rendering any glyphs. Cursive/italic glyphs can render
  // wider than their own advance (connecting flourishes, negative right
  // side bearing), so the buffer must fit the rendered ink, not just the
  // sum of advances, or the render pass below gets clipped on the right.
  int textWidth = 0;
  int maxInkExtent = 0;
  int nrOfRows = std::count(text.begin(), text.end(), '\n') + 1;
  int tempMaxRowWidth = 0;
  vlog.debug("Number of rows: %i", nrOfRows);
  {
    int penX = 0;
    for (size_t i = 0; i < text.size(); i++) {
      if (text[i] == '\n') {
        if (penX > tempMaxRowWidth)
          tempMaxRowWidth = penX;
        penX = 0;
        continue;
      }
      if (FT_Load_Char(face, static_cast<FT_ULong>(text[i]), FT_LOAD_RENDER) !=
          0)
        continue;

      FT_GlyphSlot glyph = face->glyph;
      int inkExtent =
          penX + glyph->bitmap_left + static_cast<int>(glyph->bitmap.width);
      if (inkExtent > maxInkExtent)
        maxInkExtent = inkExtent;

      penX += glyph->advance.x >> 6;
    }
    textWidth = std::max({penX, maxInkExtent, tempMaxRowWidth});
  }
  int ascent = face->size->metrics.ascender >> 6;
  int descent = -(face->size->metrics.descender >> 6);

  // Adjust line spacing to avoid to much space between lines
  const float lineSpacing = 0.75f;
  int rowHeight = static_cast<int>((ascent + descent) * lineSpacing);
  int textHeight = ascent + descent + (nrOfRows - 1) * rowHeight;

  if ((textWidth <= 0) || (textHeight <= 0))
    return nullptr;

  int stride = textWidth * 4;
  uint8_t* buf = new uint8_t[stride * textHeight]();

  pixman_image_t* destImage =
      pixman_image_create_bits(PIXMAN_a8r8g8b8, textWidth, textHeight,
                               reinterpret_cast<uint32_t*>(buf), stride);
  if (!destImage) {
    vlog.error("Failed to create Pixman image surface wrapper for text.");
    delete[] buf;
    return nullptr;
  }

  pixman_color_t whiteColor = {0xffff, 0xffff, 0xffff, 0xffff};
  pixman_image_t* textColor = pixman_image_create_solid_fill(&whiteColor);

  int penX = 0;
  int baselineY = ascent;

  for (size_t i = 0; i < text.size(); i++) {
    if (text[i] == '\n') {
      penX = 0;
      baselineY += rowHeight;
      continue;
    }

    if (FT_Load_Char(face, static_cast<FT_ULong>(text[i]), FT_LOAD_RENDER) != 0)
      continue;

    FT_GlyphSlot glyph = face->glyph;
    FT_Bitmap& bitmap = glyph->bitmap;

    int glyphX = penX + glyph->bitmap_left;
    int glyphY = baselineY - glyph->bitmap_top;

    if ((bitmap.width > 0) && (bitmap.rows > 0)) {
      // Pixman requires the stride to be a multiple of 4 bytes, which
      // FreeType's tightly-packed 8-bit bitmaps rarely are.
      int alignedStride = (bitmap.width + 3) & ~3;
      std::vector<uint8_t> maskBuffer(alignedStride * bitmap.rows, 0);
      for (unsigned int row = 0; row < bitmap.rows; row++)
        memcpy(&maskBuffer[row * alignedStride],
               bitmap.buffer + row * bitmap.pitch, bitmap.width);

      pixman_image_t* mask = pixman_image_create_bits(
          PIXMAN_a8, bitmap.width, bitmap.rows,
          reinterpret_cast<uint32_t*>(maskBuffer.data()), alignedStride);

      if (mask) {
        pixman_image_composite(PIXMAN_OP_OVER, textColor, mask, destImage, 0, 0,
                               0, 0, glyphX, glyphY, bitmap.width, bitmap.rows);
        pixman_image_unref(mask);
      }
    }

    penX += glyph->advance.x >> 6;
  }

  pixman_image_unref(textColor);
  pixman_image_unref(destImage);

  *outWidth = textWidth;
  *outHeight = textHeight;
  return buf;
}
