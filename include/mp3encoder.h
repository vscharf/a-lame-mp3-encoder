// -*- C++ -*-
#ifndef ALAMEMP3ENCODER_MP3DECODER_H
#define ALAMEMP3ENCODER_MP3DECODER_H

#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include "lame.h"

namespace vscharf {

// ======== forward decl. ========
class WavDecoder;

// ======== exceptions ========
class lame_error : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

// ======== classes ========
// Encodes output from WavDecoder to mp3 using lame.
// Objects of this class are not thread-safe.
class Mp3Encoder {
public:
  using char_buffer = std::string;

  // lame quality setting: 0 = best ... 9 = worst
  Mp3Encoder(int quality);
  ~Mp3Encoder();
  Mp3Encoder(const Mp3Encoder&) = delete;
  const Mp3Encoder& operator=(const Mp3Encoder&) = delete;
  Mp3Encoder(Mp3Encoder&&) = delete;
  const Mp3Encoder& operator=(Mp3Encoder&&) = delete;

  // Encode the PCM data from in to the ostream output using nsamples at once.
  // By default nsamples is adjusted such that 4K bytes are read at once.
  void encode(WavDecoder& in, std::ostream& output, uint32_t nsamples = 0);

private:
  lame_global_flags* gfp_;
  int quality_;
  char_buffer buf_;
};

} // namespace vscharf


#endif //ALAMEMP3ENCODER_MP3DECODER_H
