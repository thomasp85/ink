#include "ink.h"
#include "InkDevice.h"
#include "init_device.h"

class InkDeviceBmp : public InkDevice {
public:
  InkDeviceBmp(const char* fp, int w, int h, double ps, int bg, double res) :
  InkDevice(fp, w, h, ps, bg, res)
  {

  }
  // Behaviour
  bool savePage() {
    char buf[PATH_MAX+1];
    snprintf(buf, PATH_MAX, this->file.c_str(), this->pageno); buf[PATH_MAX] = '\0';
    BLImageCodec codec;
    codec.findByName("BMP");
    BLResult res = canvas.writeToFile(buf, codec);
    return res == BL_SUCCESS;
  };
};

// [[export]]
SEXP ink_bmp_c(SEXP file, SEXP width, SEXP height, SEXP pointsize, SEXP bg,
               SEXP res) {
  int bgCol = RGBpar(bg, 0);
  InkDeviceBmp* device = new InkDeviceBmp(
    CHAR(STRING_ELT(file, 0)),
    INTEGER(width)[0],
    INTEGER(height)[0],
    REAL(pointsize)[0],
    bgCol,
    REAL(res)[0]
  );
  makeInkDevice<InkDeviceBmp>(device, "ink_bmp");

  return R_NilValue;
}