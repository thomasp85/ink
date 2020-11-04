#include <R.h>
#include <Rinternals.h>
#include <stdlib.h> // for NULL
#include <R_ext/Rdynload.h>

#include "ink.h"

static font_map* fonts;

font_map& get_font_map(){
  return *fonts;
}

static const R_CallMethodDef CallEntries[] = {
  {"ink_bmp_c", (DL_FUNC) &ink_bmp_c, 6},
  {NULL, NULL, 0}
};

extern "C" void R_init_ink(DllInfo *dll) {
  fonts = new font_map();

  R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
  R_useDynamicSymbols(dll, FALSE);
}

extern "C" void R_unload_ink(DllInfo *dll) {
  delete fonts;
}
