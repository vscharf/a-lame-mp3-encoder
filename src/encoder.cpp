// This struct implements RAII for single instance of a lame encoder.
#include <ostream>
#include <stdexcept>
#include <string>

#include "lame.h"

#include "wavdecoder.h"

using namespace vscharf;

class lame_error : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

struct LameEncoder {
public:
  using char_buffer = std::string;

  LameEncoder(int quality);
  ~LameEncoder();
  LameEncoder(const LameEncoder&) = delete;
  const LameEncoder& operator=(const LameEncoder&) = delete;
  LameEncoder(LameEncoder&&) = delete;
  const LameEncoder& operator=(LameEncoder&&) = delete;

  void encode(WavDecoder& in, std::ostream& output, uint32_t nsamples = 0);

private:
  lame_global_flags* gfp_;
  int quality_;
  char_buffer buf_;
};

LameEncoder::LameEncoder(int quality)
  : gfp_(lame_init())
  , quality_(quality)
{
  if(!gfp_) throw lame_error("Call to lame_init() failed!");
}

LameEncoder::~LameEncoder()
{
  lame_close(gfp_);
}

// namespace {
//   convert_to_payload_type
// }

// Encode the data from in to an ostream out taking nsamples at
// once. If nsamples is zero it will be chosen such that 4k bytes will
// be processed at once.
void LameEncoder::encode(WavDecoder& in, std::ostream& out, uint32_t nsamples /* = 0 */)
{
  if(!out) throw decoder_error("Invalid output stream!");

  // initialize lame
  lame_set_num_channels(gfp_, in.get_header().channels);
  lame_set_in_samplerate(gfp_, in.get_header().samplesPerSec);
  lame_set_quality(gfp_, quality_);
  if(lame_init_params(gfp_) < 0) throw lame_error("Lame initialization failed!");

  // auto-determine sample size
  if(!nsamples) nsamples = 4096 / in.get_header().bytesPerSample;
  buf_.resize(1.25 * nsamples + 7200); // worst-case estimate from lame/API

  // the actual encoding
  while(in.has_next()) {
    const auto& inbuf = in.read_samples(nsamples);

    auto n_read_samples = inbuf.size() / in.get_header().bytesPerSample;

    int n;
    if(in.get_header().channels > 1) {
      n = lame_encode_buffer_interleaved(gfp_,
					 const_cast<int16_t*>(&inbuf[0]),
					 n_read_samples / in.get_header().channels,
					 reinterpret_cast<unsigned char*>(&buf_[0]),
					 buf_.size());
    } else {
      n = lame_encode_buffer(gfp_,
			     const_cast<int16_t*>(&inbuf[0]),
			     nullptr,
			     n_read_samples,
			     reinterpret_cast<unsigned char*>(&buf_[0]),
			     buf_.size());
    }

    if(n < 0) throw decoder_error("lame_encode_buffer returned error!");
    if(!out.write(buf_.data(), n)) throw decoder_error("Writing to output failed!");
  }

  // flush the rest
  buf_.resize(7200);
  auto n = lame_encode_flush(gfp_, reinterpret_cast<unsigned char*>(&buf_[0]), buf_.size());
  if(n < 0) throw decoder_error("lame_encode_flush returned error!");
  if(n > 0 && !out.write(buf_.data(), n)) throw decoder_error("Writing to output failed!");
}

#ifdef TEST_ENCODER
#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
  std::string input("test_data/sound.wav");
  if(argc > 1) input = argv[1];

  std::ofstream mp3_file("test.mp3");

  try {
    WavDecoder w(input);
    LameEncoder l(5);
    l.encode(w, mp3_file);
  } catch(const std::runtime_error& d) {
    std::cerr << std::endl << "Error decoding test_data/sound.wav: " << d.what() << std::endl;
    return 1;
  }
  std::cout << "Test finished successfully!" << std::endl;
  return 0;
}
#endif // TEST_DECODER
