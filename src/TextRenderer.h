#pragma once

#include "ink.h"

#include <vector>
#include <systemfonts.h>
#include <textshaping.h>

class TextRenderer {
  FontSettings last_font;
  std::vector<textshaping::Point> loc_buffer;
  std::vector<uint32_t> id_buffer;
  std::vector<int> cluster_buffer;
  std::vector<unsigned int> font_buffer;
  std::vector<FontSettings> fallback_buffer;

  BLFontData fontdata;
  BLFontFace fontface;
  BLFont font;
  BLFontMetrics fontmetrics;

  int last_char = -1;
  BLGlyphBuffer last_char_buffer;
  BLTextMetrics last_char_metric;

public:
  TextRenderer() {}

  BLResult load_font(const char *family, int face, double size) {
    FontSettings fontfile = get_font_file(family,
                                      face == 2 || face == 4,
                                      face == 3 || face == 4,
                                      face == 5);

    BLResult err = BL_SUCCESS;

    bool refresh = false;

    if (fontfile.index != last_font.index ||
        strncmp(fontfile.file, last_font.file, PATH_MAX) != 0) {
      refresh = true;
      err = fontdata.createFromFile(fontfile.file, BL_FILE_READ_MMAP_ENABLED);
      if (err != BL_SUCCESS) return err;
    }
    if (refresh || fontfile.index != last_font.index) {
      refresh = true;
      err = fontface.createFromData(fontdata, fontfile.index);
      if (err != BL_SUCCESS) return err;
    }
    last_font = fontfile;
    if (refresh || font.size() != (float) size) {
      refresh = true;
      err = font.createFromFace(fontface, size);
      if (err != BL_SUCCESS) return err;
      fontmetrics = font.metrics();
    }
    if (refresh) {
      last_char = -1;
      last_char_buffer.clear();
      last_char_metric.reset();
    }

    return err;
  }

  double get_text_width(const char* string) {
    double width = 0.0;
    int error = textshaping::string_width(
      string,
      last_font,
      font.size(),
      72.0,
      1,
      &width
    );
    if (error) {
      return 0.0;
    }
    return width;
  }

  void get_char_metric(int c, double *ascent, double *descent, double *width) {
    load_char(c);
    *width = last_char_metric.boundingBox.x1 - last_char_metric.boundingBox.x0;
    // TODO Use glyph metrics instead of font metrics
    *ascent = fontmetrics.ascent;
    *descent = fontmetrics.descent;
  }

  void plot_text(double x, double y, const char *string, double rot, double hadj,
                 BLContext &context) {
    double width = get_text_width(string);

    if (width == 0.0) {
      return;
    }

    int expected_max = strlen(string) * 16;
    loc_buffer.reserve(expected_max);
    id_buffer.reserve(expected_max);
    cluster_buffer.reserve(expected_max);
    font_buffer.reserve(expected_max);
    fallback_buffer.reserve(expected_max);

    int err = textshaping::string_shape(
      string,
      last_font,
      font.size(),
      72.0,
      loc_buffer,
      id_buffer,
      cluster_buffer,
      font_buffer,
      fallback_buffer
    );

    if (err != 0) {
      Rf_warning("textshaping failed to shape the string");
      return;
    }

    int n_glyphs = loc_buffer.size();

    if (n_glyphs == 0) {
      return;
    }

    BLGlyphRun gr = {};
    gr.glyphData = id_buffer.data();
    gr.placementData = loc_buffer.data();
    gr.size = n_glyphs;
    gr.glyphSize = sizeof(uint32_t);
    gr.glyphAdvance = sizeof(uint32_t);
    gr.placementType = BL_GLYPH_PLACEMENT_TYPE_USER_UNITS;
    gr.placementAdvance = sizeof(textshaping::Point);

    rot = -rot * DEG_TO_RAD;
    x -= (width * hadj) * cos(rot);
    y -= (width * hadj) * sin(rot);

    if (rot != 0) {
      context.rotate(rot, x, y);
    }

    context.fillGlyphRun(BLPoint(x, y), font, gr);

    if (rot != 0) {
      context.resetMatrix();
    }
  }

private:
  void load_char(int code) {
    if (code != last_char) {
      std::wstring c(static_cast<wchar_t>(code), 1);
      last_char_buffer.setWCharText(c.c_str(), 1);
      font.shape(last_char_buffer);
      font.getTextMetrics(last_char_buffer, last_char_metric);

      last_char = code;
    }
  }
  FontSettings get_font_file(const char* family, int bold, int italic,
                             int symbol) {
    const char* fontfamily = family;
    if (symbol) {
#if defined _WIN32
      fontfamily = "Segoe UI Symbol";
#else
      fontfamily = "Symbol";
#endif
    }
    return locate_font_with_features(fontfamily, italic, bold);
  }
};
