#ifndef INK_INCLUDED
#define INK_INCLUDED

#include <string>
#include <tuple>
#include <utility>
#include <unordered_map>

#include <R.h>
#include <Rinternals.h>
#include <R_ext/GraphicsEngine.h>

#include <blend2d.h>

const double DEG_TO_RAD = 0.0174533;

typedef std::tuple<std::string, int, int> font_key;

struct key_hash : public std::unary_function<font_key, std::size_t>
{
  inline std::size_t operator()(const font_key& k) const
  {
    return std::get<0>(k)[0] ^ std::get<1>(k) ^ std::get<2>(k);
  }
};

struct key_equal : public std::binary_function<font_key, font_key, bool>
{
  inline bool operator()(const font_key& v0, const font_key& v1) const
  {
    return (
        std::get<0>(v0) == std::get<0>(v1) &&
          std::get<1>(v0) == std::get<1>(v1) &&
          std::get<2>(v0) == std::get<2>(v1)
    );
  }
};

typedef std::unordered_map<font_key, std::pair< std::string, int >, key_hash, key_equal> font_map;

SEXP ink_bmp_c(SEXP file, SEXP width, SEXP height, SEXP pointsize, SEXP bg,
               SEXP res);

#endif