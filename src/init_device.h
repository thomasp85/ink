#pragma once

#include "ink.h"

template<class T>
void ink_metric_info(int c, const pGEcontext gc, double* ascent,
                     double* descent, double* width, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->charMetric(c, gc->fontfamily, gc->fontface, gc->ps * gc->cex,
                     ascent, descent, width);
  return;
}

template<class T>
void ink_clip(double x0, double x1, double y0, double y1, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->clipRect(x0, y0, x1, y1);
}

template<class T>
void ink_new_page(const pGEcontext gc, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->newPage();
  return;
}

template<class T>
void ink_close(pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->close();
  delete device;
  return;
}

template<class T>
void ink_line(double x1, double y1, double x2, double y2,
              const pGEcontext gc, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawLine(x1, y1, x2, y2, gc->col, gc->lwd, gc->lty, gc->lend);
  return;
}

template<class T>
void ink_polyline(int n, double *x, double *y, const pGEcontext gc,
                  pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawPolyline(n, x, y, gc->col, gc->lwd, gc->lty, gc->lend, gc->ljoin,
                       gc->lmitre);
  return;
}

template<class T>
void ink_polygon(int n, double *x, double *y, const pGEcontext gc,
                 pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawPolygon(n, x, y, gc->fill, gc->col, gc->lwd, gc->lty, gc->lend,
                      gc->ljoin, gc->lmitre);
  return;
}

template<class T>
void ink_path(double *x, double *y, int npoly, int *nper, Rboolean winding,
              const pGEcontext gc, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawPath(npoly, nper, x, y, gc->col, gc->fill, gc->lwd, gc->lty,
                   gc->lend, gc->ljoin, gc->lmitre, !winding);
  return;
}

template<class T>
double ink_strwidth(const char *str, const pGEcontext gc, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  return device->stringWidth(str, gc->fontfamily, gc->fontface,
                             gc->ps * gc->cex);
}

template<class T>
void ink_rect(double x0, double y0, double x1, double y1, const pGEcontext gc,
              pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawRect(x0, y0, x1, y1, gc->fill, gc->col, gc->lwd,
                   gc->lty, gc->lend);
  return;
}

template<class T>
void ink_circle(double x, double y, double r, const pGEcontext gc,
                pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawCircle(x, y, r, gc->fill, gc->col, gc->lwd, gc->lty, gc->lend);
  return;
}

template<class T>
void ink_text(double x, double y, const char *str, double rot, double hadj,
              const pGEcontext gc, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawText(x, y, str, gc->fontfamily, gc->fontface, gc->ps * gc->cex,
                   rot, hadj, gc->col);
  return;
}

static void ink_size(double *left, double *right, double *bottom, double *top,
                     pDevDesc dd) {
  *left = dd->left;
  *right = dd->right;
  *bottom = dd->bottom;
  *top = dd->top;
}

template<class T>
void ink_raster(unsigned int *raster, int w, int h, double x, double y,
                double width, double height, double rot, Rboolean interpolate,
                const pGEcontext gc, pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  device->drawRaster(raster, w, h, x, y, width, height, rot, interpolate);
  return;
}

template<class T>
SEXP ink_capture(pDevDesc dd) {
  T * device = (T *) dd->deviceSpecific;
  return device->capture();
}

template<class T>
pDevDesc ink_device_new(T* device) {

  pDevDesc dd = (DevDesc*) calloc(1, sizeof(DevDesc));
  if (dd == NULL)
    return dd;

  dd->startfill = device->background_int;
  dd->startcol = R_RGB(0, 0, 0);
  dd->startps = device->pointsize;
  dd->startlty = LTY_SOLID;
  dd->startfont = 1;
  dd->startgamma = 1;

  // Callbacks
  dd->activate = NULL;
  dd->deactivate = NULL;
  dd->close = ink_close<T>;
  dd->clip = ink_clip<T>;
  dd->size = ink_size;
  dd->newPage = ink_new_page<T>;
  dd->line = ink_line<T>;
  dd->text = ink_text<T>;
  dd->strWidth = ink_strwidth<T>;
  dd->rect = ink_rect<T>;
  dd->circle = ink_circle<T>;
  dd->polygon = ink_polygon<T>;
  dd->polyline = ink_polyline<T>;
  dd->path = ink_path<T>;
  dd->mode = NULL;
  dd->metricInfo = ink_metric_info<T>;
  if (device->can_capture) {
    dd->cap = ink_capture<T>;
  } else {
    dd->cap = NULL;
  }
  dd->raster = ink_raster<T>;

  // UTF-8 support
  dd->wantSymbolUTF8 = (Rboolean) 1;
  dd->hasTextUTF8 = (Rboolean) 1;
  dd->textUTF8 = ink_text<T>;
  dd->strWidthUTF8 = ink_strwidth<T>;

  // Screen Dimensions in pts
  dd->left = 0.0;
  dd->top = 0.0;
  dd->right = device->width;
  dd->bottom = device->height;

  // Magic constants copied from other graphics devices
  // nominal character sizes in pts
  dd->cra[0] = 0.9 * device->pointsize * device->res_mod;
  dd->cra[1] = 1.2 * device->pointsize * device->res_mod;
  // character alignment offsets
  dd->xCharOffset = 0.4900;
  dd->yCharOffset = 0.3333;
  dd->yLineBias = 0.2;
  // inches per pt
  dd->ipr[0] = 1.0 / (72 * device->res_mod);
  dd->ipr[1] = 1.0 / (72 * device->res_mod);

  // Capabilities
  dd->canClip = TRUE;
  dd->canHAdj = 2;
  dd->canChangeGamma = FALSE;
  dd->displayListOn = FALSE;
  dd->haveTransparency = 2;
  dd->haveTransparentBg = 2;
  dd->useRotatedTextInContour =  (Rboolean) 1;

  dd->deviceSpecific = device;

  return dd;
}

template<class T>
void makeInkDevice(T* device, const char *name) {
  R_GE_checkVersionOrDie(R_GE_version);
  R_CheckDeviceAvailable();
  BEGIN_SUSPEND_INTERRUPTS {
    pDevDesc dev = ink_device_new<T>(device);
    if (dev == NULL)
      Rf_error("ink device failed to open");

    pGEDevDesc dd = GEcreateDevDesc(dev);
    GEaddDevice2(dd, name);
    GEinitDisplayList(dd);

  } END_SUSPEND_INTERRUPTS;
}
