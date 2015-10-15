// -*- C++ -*-
#ifndef ALAMEMP3ENCODER_HELPER_H
#define ALAMEMP3ENCODER_HELPER_H

#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

namespace vscharf {

// ======== exceptions ========
class posix_error : public std::exception {
public:
  posix_error(int err) : _err(err) {}
  virtual const char* what() const noexcept override;
private:
  int _err;
};

// ======== functions ========
std::vector<std::string> directory_entries(std::string path);
// [ argument taken by value as it is modified inside the function ]
} // namespace vscharf

#endif // ALAMEMP3ENCODER_HELPER_H
