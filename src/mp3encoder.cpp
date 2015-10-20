// This struct implements RAII for single instance of a lame encoder.
#include "mp3encoder.h"

#include <ostream>
#include "wavdecoder.h"

using namespace vscharf;

Mp3Encoder::Mp3Encoder(int quality)
  : gfp_(lame_init())
  , quality_(quality)
{
  if(!gfp_) throw lame_error("Call to lame_init() failed!");
}

Mp3Encoder::~Mp3Encoder()
{
  lame_close(gfp_);
}

// Encode the data from in to an ostream out taking nsamples at
// once. If nsamples is zero it will be chosen such that 4k bytes will
// be processed at once.
void Mp3Encoder::encode(WavDecoder& in, std::ostream& out, uint32_t nsamples /* = 0 */)
{
  if(!out) throw decoder_error("Invalid output stream!");

  // initialize lame
  lame_set_num_channels(gfp_, in.get_header().channels);
  lame_set_in_samplerate(gfp_, in.get_header().samplesPerSec);
  lame_set_quality(gfp_, quality_);
  if(lame_init_params(gfp_) < 0) throw lame_error("lame initialization failed!");

  // auto-determine sample size
  if(!nsamples) nsamples = 4096 / in.get_header().bytesPerSample;
  buf_.resize(1.25 * nsamples + 7200); // worst-case estimate from lame/API

  // the actual encoding
  while(in.has_next()) {
    const auto& inbuf = in.read_samples(nsamples);

    int n;
    if(in.get_header().channels > 1) {
      n = lame_encode_buffer_interleaved(gfp_,
					 const_cast<int16_t*>(&inbuf[0]),
					 inbuf.size() / in.get_header().channels,
					 reinterpret_cast<unsigned char*>(&buf_[0]),
					 buf_.size());
    } else {
      n = lame_encode_buffer(gfp_,
			     const_cast<int16_t*>(&inbuf[0]),
			     nullptr,
			     inbuf.size(),
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
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{
  std::string input("test_data/sound.wav");
  if(argc > 1) input = argv[1];

  std::ifstream wav_file(input);
  std::stringstream output;

  try {
    WavDecoder w(wav_file);
    Mp3Encoder l(2);
    l.encode(w, output);
  } catch(const std::runtime_error& d) {
    std::cerr << std::endl << "Error decoding test_data/sound.wav: " << d.what() << std::endl;
    return 1;
  }

  std::string bytes(4, 0);

  // header of mp3 file
  output.read(&bytes[0], 4);
  assert((bytes == std::string{'\xff', '\xfb', '\x50', '\xc4'}));

  // somewhere in the middle
  output.ignore(0x300); // seek 0x304 bytes into the file
  output.read(&bytes[0], 4);
  assert((bytes == std::string{'\xfa', '\xbd', '\x5f', '\x51'}));

  // (almost) last bytes
  output.ignore(0x1428); // seek 0x1730 bytes into the file
  output.read(&bytes[0], 4);
  assert((bytes == std::string{'\x0d', '\x5f', '\xed', '\x4a'}));

  std::cout << "Test finished successfully!" << std::endl;
  return 0;
}
#endif // TEST_DECODER
