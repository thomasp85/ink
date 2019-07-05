#include <vector>

#include "ink.h"
#include "fonts.h"

#ifndef TEXTREN_INCLUDED
#define TEXTREN_INCLUDED

class TextRenderer {
  std::pair<std::string, int> last_font;
  BLFontLoader fontloader;
  BLFontFace fontface;
  BLFont font;
  BLFontMetrics fontmetrics;

  std::string last_string = "";
  BLGlyphBuffer last_string_buffer;
  BLTextMetrics last_string_metric;

  int last_char = -1;
  BLGlyphBuffer last_char_buffer;
  BLTextMetrics last_char_metric;

  double deg_to_rad = 0.0174533;

public:
  TextRenderer() {
    last_font = std::make_pair("", -1);
  }

  BLResult load_font(const char *family, int face, double size) {
    std::pair<std::string, int> fontfile = get_font_file("Arial", //Currently troubles with ttc files
                                                         face == 2 || face == 4,
                                                         face == 3 || face == 4,
                                                         face == 5);

    BLResult err;

    bool refresh = false;

    if (fontfile.first != last_font.first) {
      refresh = true;
      err = fontloader.createFromFile(fontfile.first.c_str(),
                                      BL_FILE_READ_MMAP_ENABLED);
      if (err != BL_SUCCESS) return err;
    }
    if (refresh || fontfile.second != last_font.second) {
      refresh = true;
      err = fontface.createFromLoader(fontloader, fontfile.second);
      if (err != BL_SUCCESS) return err;
    }
    last_font = fontfile;
    if (refresh || font.size() != (float) size) {
      refresh = true;
      err = font.createFromFace(fontface, size);
      fontmetrics = font.metrics();
    }
    if (refresh) {
      last_string = "";
      last_string_buffer.clear();
      last_string_metric.reset();

      last_char = -1;
      last_char_buffer.clear();
      last_char_metric.reset();
    }

    return err;
  }

  double get_text_width(const char* string) {
    load_string(string);
    return last_string_metric.advance.x;
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
    load_string(string);
    double width = last_string_metric.advance.x;

    rot = -rot * deg_to_rad;
    x -= (width * hadj) * cos(rot);
    y -= (width * hadj) * sin(rot);

    if (rot != 0) {
      context.rotate(rot, x, y);
    }


    context.fillGlyphRun(BLPoint(x, y), font, last_string_buffer.glyphRun());

    if (rot != 0) {
      context.resetMatrix();
    }
  }

private:
  void load_string(const char * text) {
    if (strcmp(text, last_string.c_str()) != 0) {
      last_string_buffer.setUtf8Text(text);
      font.shape(last_string_buffer);
      font.applyKerning(last_string_buffer);
      font.getTextMetrics(last_string_buffer, last_string_metric);

      last_string = text;
    }
  }
  void load_char(int code) {
    if (code != last_char) {
      std::wstring c(static_cast<wchar_t>(code), 1);
      last_char_buffer.setWCharText(c.c_str(), 1);
      font.shape(last_char_buffer);
      font.getTextMetrics(last_char_buffer, last_char_metric);

      last_char = code;
    }
  }
};

#endif
