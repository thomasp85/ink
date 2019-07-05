#' Draw to a bmp file
#'
#' The BMP (bitmap) format is an image format developed by Microsoft to store
#' raster images. It is relatively simple and well documented making it a
#' popular lower bound format. In general, PNG is now prefered over BMP if
#' possible.
#'
#' @param filename The name of the file. Follows the same semantics as the file
#'   naming in [grDevices::png()], meaning that you can provide a [sprintf()]
#'   compliant string format to name multiple plots (such as the default value)
#' @param width,height The dimensions of the device
#' @param units The unit `width` and `height` is measured in, in either pixels
#'   (`'px'`), inches (`'in'`), millimeters (`'mm'`), or centimeter (`'cm'`).
#' @param pointsize The default pointsize of the device in pt
#' @param background The background colour of the device
#' @param res The resolution of the device. This setting will govern how device
#'   dimensions given in inches, centimeters, or millimeters will be converted
#'   to pixels. Further, it will be used to scale text sizes and linewidths
#'
#' @export
#'
#' @examples
#' file <- tempfile(fileext = '.ppm')
#' ink_bmp(file)
#' plot(sin, -pi, 2*pi)
#' dev.off()
#'
ink_bmp <- function(filename = 'Rplot%03d.bmp', width = 480, height = 480,
                    units = 'px', pointsize = 12, background = 'white',
                    res = 72) {
  if (deparse(sys.call()) == 'dev(filename = filename, width = dim[1], height = dim[2], ...)') {
    units <- 'in'
  }
  file <- validate_path(filename)
  dim <- get_dims(width, height, units, res)
  .Call("ink_bmp_c", file, dim[1], dim[2], as.numeric(pointsize), background,
        as.numeric(res), PACKAGE = 'ink')
  invisible()
}