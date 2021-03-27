/*
  STOLEN FROM https://github.com/yhirose/cpp-httplib/blob/master/httplib.h
*/

#include <string>

inline std::string base64_encode(const std::string &in) {
  static const auto lookup =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string out;
  out.reserve(in.size());

  int val = 0;
  int valb = -6;

  for (auto c : in) {
    val = (val << 8) + static_cast<uint8_t>(c);
    valb += 8;
    while (valb >= 0) {
      out.push_back(lookup[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }

  if (valb > -6) { out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]); }

  while (out.size() % 4) {
    out.push_back('=');
  }

  return out;
}