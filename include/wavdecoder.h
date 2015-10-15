// -*- C++ -*-
#ifndef ALAMEMP3ENCODER_WAVDECODER_H
#define ALAMEMP3ENCODER_WAVDECODER_H

#include <cstdint>
#include <istream>
#include <stdexcept>
#include <vector>

namespace vscharf {

// ======== exceptions ========
class decoder_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

// ======== classes ========
// Reads RIFF/WAVE files and decodes them into 16-bit signed PCM data.
// Objects of this class are not thread-safe.
class WavDecoder {
public:
  using char_buffer = std::vector<int16_t>;

  struct WavHeader {
    uint16_t channels;
    uint32_t samplesPerSec;
    uint32_t avgBytesPerSec;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t bytesPerSample;
  };

  WavDecoder(std::istream& in);
  
  const WavHeader& get_header() const { return header_; }
  bool has_next() const { in_.peek(); return in_.good(); }
  // make sure eof is triggered --^

  // Read up to nsamples samples (default = 1). The size of
  // char_buffer will represent the actual number of samples read. The
  // reference is valid up to the next call to read_samples.
  // The buffer contains 16-bit resolution PCM samples.
  const char_buffer& read_samples(uint32_t nsamples);

private:
  void decode_wav_header();
  void skip_chunk();
  void seek_data();

  WavHeader header_;
  std::istream& in_; // mutable to allow has_next to peek
  char_buffer buf_;
  uint32_t remaining_chunk_size_ = 0;
};

} // namespace vscharf

#endif // ALAMEMP3ENCODER_WAVDECODER_H
