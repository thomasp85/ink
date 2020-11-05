#pragma once

#include "ink.h"
#include "TextRenderer.h"

/* Base class for graphic device interface to Blend2D.
 *
 * Specific devices should subclass this and provide their own canvas and
 * savePage() methods (at least), while the drawing methods should work
 * regardless.
 */
class InkDevice {
public:
  BLImage canvas;
  BLContext context;

  bool can_capture = false;

  int width;
  int height;
  double clip_left;
  double clip_right;
  double clip_top;
  double clip_bottom;

  int pageno;
  std::string file;
  int background_int;
  double pointsize;
  double res_real;
  double res_mod;
  double lwd_mod;

  TextRenderer text_renderer;

  // Lifecycle methods
  InkDevice(const char* fp, int w, int h, double ps, int bg, double res,
            double scaling);
  virtual ~InkDevice();
  void newPage(unsigned int bg, bool increase_pageno = true);
  void close();
  virtual bool savePage();
  SEXP capture();

  // Behaviour
  void clipRect(double x0, double y0, double x1, double y1);
  double stringWidth(const char *str, const char *family, int face,
                     double size);
  void charMetric(int c, const char *family, int face, double size,
                  double *ascent, double *descent, double *width);

  // Drawing Methods
  void drawCircle(double x, double y, double r, int fill, int col, double lwd,
                  int lty, R_GE_lineend lend);
  void drawRect(double x0, double y0, double x1, double y1, int fill, int col,
                double lwd, int lty, R_GE_lineend lend);
  void drawPolygon(int n, double *x, double *y, int fill, int col, double lwd,
                   int lty, R_GE_lineend lend, R_GE_linejoin ljoin,
                   double lmitre);
  void drawLine(double x1, double y1, double x2, double y2, int col, double lwd,
                int lty, R_GE_lineend lend);
  void drawPolyline(int n, double* x, double* y, int col, double lwd, int lty,
                    R_GE_lineend lend, R_GE_linejoin ljoin, double lmitre);
  void drawPath(int npoly, int* nper, double* x, double* y, int col, int fill,
                double lwd, int lty, R_GE_lineend lend, R_GE_linejoin ljoin,
                double lmitre, bool evenodd);
  void drawRaster(unsigned int *raster, int w, int h, double x, double y,
                  double final_width, double final_height, double rot,
                  bool interpolate);
  void drawText(double x, double y, const char *str, const char *family,
                int face, double size, double rot, double hadj, int col);

private:
  unsigned int col_cur = R_GE_str2col("black");
  unsigned int fill_cur = R_GE_str2col("black");
  int lty_cur = -2;
  double lwd_cur = -1.0;
  R_GE_lineend lend_cur = GE_BUTT_CAP;
  R_GE_linejoin ljoin_cur = GE_MITRE_JOIN;
  double mitre_cur = -1.0;

  inline BLRgba32 convertColour(unsigned int col) {
    return BLRgba32(R_RED(col), R_GREEN(col), R_BLUE(col), R_ALPHA(col));
  }
  inline bool visibleColour(unsigned int col) {
    return (int) R_ALPHA(col) != 0;
  }
  inline BLStrokeCap convertLineend(R_GE_lineend lend) {
    switch (lend) {
    case GE_ROUND_CAP:
      return BL_STROKE_CAP_ROUND;
    case GE_BUTT_CAP:
      return BL_STROKE_CAP_BUTT;
    case GE_SQUARE_CAP:
      return BL_STROKE_CAP_SQUARE;
    }

    //should never happen
    return BL_STROKE_CAP_SQUARE;
  }
  inline BLStrokeJoin convertLinejoin(R_GE_linejoin ljoin) {
    switch(ljoin) {
    case GE_ROUND_JOIN:
      return BL_STROKE_JOIN_ROUND;
    case GE_MITRE_JOIN:
      return BL_STROKE_JOIN_MITER_CLIP;
    case GE_BEVEL_JOIN:
      return BL_STROKE_JOIN_BEVEL;
    }

    //should never happen
    return BL_STROKE_JOIN_ROUND;
  }
  inline BLArray<double> convertLinetype(int lty, double lwd) {
    BLArray<double> pattern;
    if (lty == LTY_SOLID) return(pattern);
    double dash, gap;
    for(int i = 0 ; i < 8 && lty & 15 ; i += 2) {
      dash = (lty & 15) * lwd;
      lty = lty>>4;
      gap = (lty & 15) * lwd;
      lty = lty>>4;
      pattern.append(dash, gap);
    }
    return pattern;
  }
  inline void setColour(unsigned int col) {
    if (col != col_cur) {
      context.setStrokeStyle(convertColour(col));
      col_cur = col;
    }
  }
  inline void setFill(unsigned int fill) {
    if (fill != fill_cur) {
      context.setFillStyle(convertColour(fill));
      fill_cur = fill;
    }
  }
  // Must be called before setLinetype
  inline void setLinewidth(double lwd) {
    if (lwd != lwd_cur) {
      lwd_cur = lwd;
      lwd *= lwd_mod;
      context.setStrokeWidth(lwd);
      lty_cur = -2; // Invalidates calculated dash;
    }
  }
  inline void setLinetype(int lty) {
    if (lty != lty_cur) {
      context.setStrokeDashArray(convertLinetype(lty, lwd_cur));
      lty_cur = lty;
    }
  }
  inline void setLineend(R_GE_lineend lend) {
    if (lend != lend_cur) {
      context.setStrokeCaps(convertLineend(lend));
      lend_cur = lend;
    }
  }
  inline void setLinejoin(R_GE_linejoin ljoin) {
    if (ljoin != ljoin_cur) {
      context.setStrokeJoin(convertLinejoin(ljoin));
      ljoin_cur = ljoin;
    }
  }
  inline void setLinemitrelim(double lmitre) {
    if (lmitre != mitre_cur) {
      context.setStrokeMiterLimit(lmitre);
      mitre_cur = lmitre;
    }
  }
  void convertRasterBuffer(unsigned int* dest, unsigned int* src, int size) {
    uint16_t r, g, b, a;
    for (int i = 0; i < size; i++){
      a = R_ALPHA(src[i]);
      if (a == 0) {
        dest[i] = 0;
        continue;
      }
      r = R_RED(src[i]) * a;
      r = (r + 1 + ((r + 1) >> 8)) >> 8;
      g = R_GREEN(src[i]) * a;
      g = (g + 1 + ((g + 1) >> 8)) >> 8;
      b = R_BLUE(src[i]) * a;
      b = (b + 1 + ((b + 1) >> 8)) >> 8;
      dest[i] = ((b)|((g)<<8)|((r)<<16)|((a)<<24));
    }
  }
  const char* blresult_string(BLResult code);
};

// IMPLIMENTATION --------------------------------------------------------------

// LIFECYCLE -------------------------------------------------------------------

/* The initialiser takes care of setting up the buffer, and caching a pixel
 * formatter and renderer.
 */
InkDevice::InkDevice(const char* fp, int w, int h, double ps, int bg,
                     double res, double scaling) :
  canvas(w, h, BL_FORMAT_PRGB32),
  context(canvas),
  width(w),
  height(h),
  pageno(0),
  file(fp),
  background_int(bg),
  pointsize(ps),
  res_real(res),
  res_mod(scaling * res / 72.0),
  lwd_mod(scaling * res / 96.0),
  text_renderer()
{
  newPage(bg, false);
}
InkDevice::~InkDevice() {
  context.end();
}
/* newPage() should not need to be overwritten as long the class have an
 * appropriate savePage() method. For scrren devices it may make sense to change
 * it for performance
 */
void InkDevice::newPage(unsigned int bg, bool increase_pageno) {
  if (pageno != 0) {
    if (!savePage()) {
      Rf_warning("ink could not write to the given file");
    }
  }

  clipRect(0, 0, width, height);
  context.setCompOp(BL_COMP_OP_SRC_COPY);
  if (visibleColour(bg)) {
    setFill(bg);
  } else {
    setFill(background_int);
  }
  context.fillAll();
  context.setCompOp(BL_COMP_OP_SRC_OVER);

  if (increase_pageno) pageno++;
}
void InkDevice::close() {
  if (pageno == 0) pageno++;
  if (!savePage()) {
    Rf_warning("ink could not write to the given file");
  }
}

SEXP InkDevice::capture() {
  // TODO
  return Rf_allocVector(INTSXP, 0);
}

/* This takes care of writing the BLImage to an appropriate file. The filename
 * may be specified as a printf string with room for a page counter, so the
 * method should take care of resolving that together with the pageno field.
 */
bool InkDevice::savePage() {
  return true;
}


// BEHAVIOUR -------------------------------------------------------------------

/* The clipRect method sets clipping on the context. Clipping is cumulative in
 * B2D so need to reset first
 */
void InkDevice::clipRect(double x0, double y0, double x1, double y1) {
  clip_left = x0;
  clip_right = x1;
  clip_top = y0;
  clip_bottom = y1;
  double width = x1 - x0;
  double height = y1 - y0;
  context.restoreClipping();
  context.clipToRect(x0, y0, width, height);
}

/* These methods funnel all operations to the text_renderer. See text_renderer.h
 * for implementation details.
 */
double InkDevice::stringWidth(const char *str, const char *family, int face,
                              double size) {
  BLResult err = text_renderer.load_font(family, face, size * res_mod);
  if (err != BL_SUCCESS) {
    Rf_warning("ink failed to load font: '%s' (%s)", family, blresult_string(err));
    return 0;
  }
  return text_renderer.get_text_width(str);
}
void InkDevice::charMetric(int c, const char *family, int face, double size,
                           double *ascent, double *descent, double *width) {
  if (c < 0) {
    c = -c;
  }

  BLResult err = text_renderer.load_font(family, face, size * res_mod);
  if (err != BL_SUCCESS) {
    Rf_warning("ink failed to load font: '%s' (%s)", family, blresult_string(err));
    *ascent = 0;
    *descent = 0;
    *width = 0;
  } else {
    text_renderer.get_char_metric(c, ascent, descent, width);
  }
}


// DRAWING ---------------------------------------------------------------------

/* Draws a circle. Used for standard points as well as grid.circle etc.
 * Can we simplify for small radius?
 */
void InkDevice::drawCircle(double x, double y, double r, int fill, int col,
                           double lwd, int lty, R_GE_lineend lend) {
  bool draw_fill = visibleColour(fill);
  bool draw_stroke = visibleColour(col) && lwd > 0.0 && lty != LTY_BLANK;

  if (!draw_fill && !draw_stroke) return; // Early exit

  r = r < 0.5 ? 0.5 : r;

  BLCircle circle(x, y, r);
  if (draw_fill) {
    setFill(fill);
    context.fillCircle(circle);
  }
  if (draw_stroke) {
    setColour(col);
    setLinewidth(lwd);
    setLineend(lend);
    setLinetype(lty);
    context.strokeCircle(circle);
  }
}

void InkDevice::drawRect(double x0, double y0, double x1, double y1, int fill,
                         int col, double lwd, int lty, R_GE_lineend lend) {
  bool draw_fill = visibleColour(fill);
  bool draw_stroke = visibleColour(col) && lwd > 0.0 && lty != LTY_BLANK;

  if (!draw_fill && !draw_stroke) return; // Early exit

  BLBox rect(x0, y0, x1, y1);
  if (draw_fill) {
    // TODO: Pixel align fill
    setFill(fill);
    context.fillBox(rect);
  }
  if (draw_stroke) {
    setColour(col);
    setLinewidth(lwd);
    setLineend(lend);
    setLinejoin(GE_MITRE_JOIN);
    setLinemitrelim(5);
    setLinetype(lty);
    context.strokeBox(rect);
  }
}

void InkDevice::drawPolygon(int n, double *x, double *y, int fill, int col,
                            double lwd, int lty, R_GE_lineend lend,
                            R_GE_linejoin ljoin, double lmitre) {
  bool draw_fill = visibleColour(fill);
  bool draw_stroke = visibleColour(col) && lwd > 0.0 && lty != LTY_BLANK;

  if (n < 2 || (!draw_fill && !draw_stroke)) return; // Early exit

  BLPath poly;
  poly.reserve(n + 1);
  poly.moveTo(x[0], y[0]);
  for (int i = 1; i < n; i++) {
    poly.lineTo(x[i], y[i]);
  }
  poly.close();

  if (draw_fill) {
    setFill(fill);
    context.fillPath(poly);
  }
  if (draw_stroke) {
    setColour(col);
    setLinewidth(lwd);
    setLineend(lend);
    setLinejoin(ljoin);
    setLinemitrelim(lmitre);
    setLinetype(lty);
    context.strokePath(poly);
  }
}

void InkDevice::drawLine(double x1, double y1, double x2, double y2, int col,
                         double lwd, int lty, R_GE_lineend lend) {
  if (!visibleColour(col) || lwd == 0.0 || lty == LTY_BLANK) return;

  BLLine line(x1, y1, x2, y2);

  setColour(col);
  setLinewidth(lwd);
  setLineend(lend);
  setLinetype(lty);
  context.strokeLine(line);
}

void InkDevice::drawPolyline(int n, double* x, double* y, int col, double lwd,
                             int lty, R_GE_lineend lend, R_GE_linejoin ljoin,
                             double lmitre) {
  if (!visibleColour(col) || lwd == 0.0 || lty == LTY_BLANK || n < 2) return;

  BLPath poly;
  poly.reserve(n);
  poly.moveTo(x[0], y[0]);
  for (int i = 1; i < n; i++) {
    poly.lineTo(x[i], y[i]);
  }

  setColour(col);
  setLinewidth(lwd);
  setLineend(lend);
  setLinejoin(ljoin);
  setLinemitrelim(lmitre);
  setLinetype(lty);
  context.strokePath(poly);
}

void InkDevice::drawPath(int npoly, int* nper, double* x, double* y, int col,
                         int fill, double lwd, int lty, R_GE_lineend lend,
                         R_GE_linejoin ljoin, double lmitre, bool evenodd) {
  bool draw_fill = visibleColour(fill);
  bool draw_stroke = visibleColour(col) && lwd > 0.0 && lty != LTY_BLANK;

  if (!draw_fill && !draw_stroke) return; // Early exit

  lwd *= lwd_mod;

  BLPath path;
  int counter = 0;
  for (int i = 0; i < npoly; i++) {
    if (nper[i] < 2) {
      counter += nper[i];
      continue;
    }
    path.moveTo(x[counter], y[counter]);
    counter++;
    for (int j = 1; j < nper[i]; j++) {
      path.lineTo(x[counter], y[counter]);
      counter++;
    };
    path.close();
  }

  if (draw_fill) {
    setFill(fill);
    context.fillPath(path);
  }
  if (draw_stroke) {
    setColour(col);
    setLinewidth(lwd);
    setLineend(lend);
    setLinejoin(ljoin);
    setLinemitrelim(lmitre);
    setLinetype(lty);
    context.strokePath(path);
  }
}

void InkDevice::drawRaster(unsigned int *raster, int w, int h, double x,
                           double y, double final_width, double final_height,
                           double rot, bool interpolate) {
  unsigned int * buffer = new unsigned int[w * h];
  convertRasterBuffer(buffer, raster, w * h);
  BLImage raster_image;
  BLResult err = raster_image.createFromData(w, h, BL_FORMAT_PRGB32, buffer, w * 4);
  if (err != BL_SUCCESS) {
    Rf_warning("Failed to mount raster with: %s", blresult_string(err));
    return;
  }
  BLPattern raster_fill(raster_image, BL_EXTEND_MODE_PAD);
  raster_fill.translate(x, y + final_height);
  raster_fill.scale(final_width / (double) w, - final_height / (double) h);
  context.rotate(-rot * DEG_TO_RAD, x, y);
  context.setPatternQuality(interpolate ? BL_PATTERN_QUALITY_BILINEAR : BL_PATTERN_QUALITY_NEAREST);
  context.setFillStyle(raster_fill);
  context.fillRect(BLRect(x, y, final_width, final_height));

  // Reset context
  context.resetMatrix();
  context.setFillStyle(convertColour(col_cur));

  raster_image.reset();
  delete[] buffer;
}

void InkDevice::drawText(double x, double y, const char *str,
                         const char *family, int face, double size, double rot,
                         double hadj, int col) {
  BLResult err = text_renderer.load_font(family, face, size * res_mod);
  if (err != BL_SUCCESS) {
    Rf_warning("ink failed to load font: '%s' (%i: %s)", family, err, blresult_string(err));
    return;
  }
  setFill(col);
  text_renderer.plot_text(x, y, str, rot, hadj, context);
}

const char * InkDevice::blresult_string(BLResult code) {
  switch (code) {
  case BL_ERROR_OUT_OF_MEMORY: return "Out of memory [ENOMEM].";
  case BL_ERROR_INVALID_VALUE: return "Invalid value/argument [EINVAL].";
  case BL_ERROR_INVALID_STATE: return "Invalid state [EFAULT].";
  case BL_ERROR_INVALID_HANDLE: return "Invalid handle or file. [EBADF].";
  case BL_ERROR_VALUE_TOO_LARGE: return "Value too large [EOVERFLOW].";
  case BL_ERROR_NOT_INITIALIZED: return "Object not initialized.";
  case BL_ERROR_NOT_IMPLEMENTED: return "Not implemented [ENOSYS].";
  case BL_ERROR_NOT_PERMITTED: return "Operation not permitted [EPERM].";
  case BL_ERROR_IO: return "IO error [EIO].";
  case BL_ERROR_BUSY: return "Device or resource busy [EBUSY].";
  case BL_ERROR_INTERRUPTED: return "Operation interrupted [EINTR].";
  case BL_ERROR_TRY_AGAIN: return "Try again [EAGAIN].";
  case BL_ERROR_TIMED_OUT: return "Timed out [ETIMEDOUT].";
  case BL_ERROR_BROKEN_PIPE: return "Broken pipe [EPIPE].";
  case BL_ERROR_INVALID_SEEK: return "File is not seekable [ESPIPE].";
  case BL_ERROR_SYMLINK_LOOP: return "Too many levels of symlinks [ELOOP].";
  case BL_ERROR_FILE_TOO_LARGE: return "File is too large [EFBIG].";
  case BL_ERROR_ALREADY_EXISTS: return "File/directory already exists [EEXIST].";
  case BL_ERROR_ACCESS_DENIED: return "Access denied [EACCES].";
  case BL_ERROR_MEDIA_CHANGED: return "Media changed [Windows::ERROR_MEDIA_CHANGED].";
  case BL_ERROR_READ_ONLY_FS: return "The file/FS is read-only [EROFS].";
  case BL_ERROR_NO_DEVICE: return "Device doesn't exist [ENXIO].";
  case BL_ERROR_NO_ENTRY: return "Not found, no entry (fs) [ENOENT].";
  case BL_ERROR_NO_MEDIA: return "No media in drive/device [ENOMEDIUM].";
  case BL_ERROR_NO_MORE_DATA: return "No more data / end of file [ENODATA].";
  case BL_ERROR_NO_MORE_FILES: return "No more files [ENMFILE].";
  case BL_ERROR_NO_SPACE_LEFT: return "No space left on device [ENOSPC].";
  case BL_ERROR_NOT_EMPTY: return "Directory is not empty [ENOTEMPTY].";
  case BL_ERROR_NOT_FILE: return "Not a file [EISDIR].";
  case BL_ERROR_NOT_DIRECTORY: return "Not a directory [ENOTDIR].";
  case BL_ERROR_NOT_SAME_DEVICE: return "Not same device [EXDEV].";
  case BL_ERROR_NOT_BLOCK_DEVICE: return "Not a block device [ENOTBLK].";
  case BL_ERROR_INVALID_FILE_NAME: return "File/path name is invalid [n/a].";
  case BL_ERROR_FILE_NAME_TOO_LONG: return "File/path name is too long [ENAMETOOLONG].";
  case BL_ERROR_TOO_MANY_OPEN_FILES: return "Too many open files [EMFILE].";
  case BL_ERROR_TOO_MANY_OPEN_FILES_BY_OS: return "Too many open files by OS [ENFILE].";
  case BL_ERROR_TOO_MANY_LINKS: return "Too many symbolic links on FS [EMLINK].";
  case BL_ERROR_TOO_MANY_THREADS: return "Too many threads [EAGAIN].";
  case BL_ERROR_THREAD_POOL_EXHAUSTED: return "Thread pool is exhausted and couldn't acquire the requested thread count.";
  case BL_ERROR_FILE_EMPTY: return "File is empty (not specific to any OS error).";
  case BL_ERROR_OPEN_FAILED: return "File open failed [Windows::ERROR_OPEN_FAILED].";
  case BL_ERROR_NOT_ROOT_DEVICE: return "Not a root device/directory [Windows::ERROR_DIR_NOT_ROOT].";
  case BL_ERROR_UNKNOWN_SYSTEM_ERROR: return "Unknown system error that failed to translate to Blend2D result code.";
  case BL_ERROR_INVALID_ALIGNMENT: return "Invalid data alignment.";
  case BL_ERROR_INVALID_SIGNATURE: return "Invalid data signature or header.";
  case BL_ERROR_INVALID_DATA: return "Invalid or corrupted data.";
  case BL_ERROR_INVALID_STRING: return "Invalid string (invalid data of either UTF8, UTF16, or UTF32).";
  case BL_ERROR_DATA_TRUNCATED: return "Truncated data (more data required than memory/stream provides).";
  case BL_ERROR_DATA_TOO_LARGE: return "Input data too large to be processed.";
  case BL_ERROR_DECOMPRESSION_FAILED: return "Decompression failed due to invalid data (RLE, Huffman, etc).";
  case BL_ERROR_INVALID_GEOMETRY: return "Invalid geometry (invalid path data or shape).";
  case BL_ERROR_NO_MATCHING_VERTEX: return "Returned when there is no matching vertex in path data.";
  case BL_ERROR_NO_MATCHING_COOKIE: return "No matching cookie (BLContext).";
  case BL_ERROR_NO_STATES_TO_RESTORE: return "No states to restore (BLContext).";
  case BL_ERROR_IMAGE_TOO_LARGE: return "The size of the image is too large.";
  case BL_ERROR_IMAGE_NO_MATCHING_CODEC: return "Image codec for a required format doesn't exist.";
  case BL_ERROR_IMAGE_UNKNOWN_FILE_FORMAT: return "Unknown or invalid file format that cannot be read.";
  case BL_ERROR_IMAGE_DECODER_NOT_PROVIDED: return "Image codec doesn't support reading the file format.";
  case BL_ERROR_IMAGE_ENCODER_NOT_PROVIDED: return "Image codec doesn't support writing the file format.";
  case BL_ERROR_PNG_MULTIPLE_IHDR: return "Multiple IHDR chunks are not allowed (PNG).";
  case BL_ERROR_PNG_INVALID_IDAT: return "Invalid IDAT chunk (PNG).";
  case BL_ERROR_PNG_INVALID_IEND: return "Invalid IEND chunk (PNG).";
  case BL_ERROR_PNG_INVALID_PLTE: return "Invalid PLTE chunk (PNG).";
  case BL_ERROR_PNG_INVALID_TRNS: return "Invalid tRNS chunk (PNG).";
  case BL_ERROR_PNG_INVALID_FILTER: return "Invalid filter type (PNG).";
  case BL_ERROR_JPEG_UNSUPPORTED_FEATURE: return "Unsupported feature (JPEG).";
  case BL_ERROR_JPEG_INVALID_SOS: return "Invalid SOS marker or header (JPEG).";
  case BL_ERROR_JPEG_INVALID_SOF: return "Invalid SOF marker (JPEG).";
  case BL_ERROR_JPEG_MULTIPLE_SOF: return "Multiple SOF markers (JPEG).";
  case BL_ERROR_JPEG_UNSUPPORTED_SOF: return "Unsupported SOF marker (JPEG).";
  case BL_ERROR_FONT_NOT_INITIALIZED: return "Font doesn't have any data as it's not initialized.";
  case BL_ERROR_FONT_NO_MATCH: return "Font or font-face was not matched (BLFontManager).";
  case BL_ERROR_FONT_NO_CHARACTER_MAPPING: return "Font has no character to glyph mapping data.";
  case BL_ERROR_FONT_MISSING_IMPORTANT_TABLE: return "Font has missing an important table.";
  case BL_ERROR_FONT_FEATURE_NOT_AVAILABLE: return "Font feature is not available.";
  case BL_ERROR_FONT_CFF_INVALID_DATA: return "Font has an invalid CFF data.";
  case BL_ERROR_FONT_PROGRAM_TERMINATED: return "Font program terminated because the execution reached the limit.";
  case BL_ERROR_INVALID_GLYPH: return "Invalid glyph identifier.";
  }
  return "Uknown error";
}
