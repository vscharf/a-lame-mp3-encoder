#include "wavdecoder.h"

#include <iostream>

namespace vscharf {

// ======== helper functions ========
namespace {
template<typename Result>
Result read_integral(std::istream& in)
{
  static_assert(std::is_integral<Result>::value, "only integral types");
  static std::string bytes(sizeof(Result), 0); // TODO is this thread-safe (thread local statics)?
  in.read(&bytes[0], sizeof(Result));  // explictely allowed since C++11
  return *reinterpret_cast<const Result*>(bytes.data()); // TODO byte order must be little-endian
} // read_integral

template<std::size_t Width>
std::string read_string(std::istream& in)
{
  std::string bytes(Width, 0);
  in.read(&bytes[0], Width); // explictely allowed since C++11
  return bytes;
} // read_string

} // anonymous namespace

// Constructs a WavDecoder object, fills the WavHeader and seeks to
// the first data chunk.
WavDecoder::WavDecoder(const std::string wav_file) : file_(wav_file)
{
  if(!file_) throw decoder_error("Couldn't open file!");
  decode_wav_header();

  // adjust buffer size accordingly
  buf_.resize(header_.bytesPerSample, 0);
}

// Skips the next chunk of the wave file. Assumes that the ckID has
// already been read and the next 4 bytes contain the chunk size.
void WavDecoder::skip_chunk()
{
  auto chunk_size = read_integral<uint32_t>(file_);
  file_.ignore(chunk_size);
}

// Decodes the RIFF/WAVE header and fills the information into
// header_.  File Specification taken from
// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
void WavDecoder::decode_wav_header()
{
  if(read_string<4>(file_) != "RIFF") throw decoder_error("No RIFF file");
  uint32_t file_size = read_integral<uint32_t>(file_);
  if(read_string<4>(file_) != "WAVE") throw decoder_error("No WAVE type");

  while(file_.good() && read_string<4>(file_) != "fmt ") skip_chunk();
  if(!file_.good()) throw decoder_error("No format chunk");

  auto fmt_chunk_size = read_integral<uint32_t>(file_);

  if(read_integral<uint16_t>(file_) != 0x1) throw decoder_error("No PCM format");
  header_.channels = read_integral<uint16_t>(file_);
  header_.samplesPerSec = read_integral<uint32_t>(file_);
  header_.avgBytesPerSec = read_integral<uint32_t>(file_);
  header_.blockAlign = read_integral<uint16_t>(file_);
  header_.bitsPerSample = read_integral<uint16_t>(file_);

  // sample size is M-byte with M = block-align / Nchannels
  header_.bytesPerSample = header_.blockAlign / header_.channels;
  
  // skip the rest of the fmt header ...
  file_.ignore(fmt_chunk_size - 16);
}

// Moves the current read position of file_ to point to the size of a
// data chunk. Assumes the file position is directly before the
// beginning of a new chunk, i.e. the next 4 bytes are the ckID.
void WavDecoder::seek_data()
{
  file_.peek(); // make sure eof is set properly if we are at the end of the file
  while(file_.good() && read_string<4>(file_) != "data") skip_chunk();
  if(!file_) throw decoder_error("Couldn't find data chunk!");
}

// Read the next sample from the current data chunk. Seek the next
// chunk if the current chunk is finished.
const WavDecoder::char_buffer& WavDecoder::read_samples(uint32_t nsamples)
{
  if(!remaining_chunk_size_) {
    seek_data();
    if(file_.eof()) { // file finished
      buf_.resize(0);
      return buf_;
    }

    // read chunk size
    remaining_chunk_size_ = read_integral<uint32_t>(file_);
    if(!remaining_chunk_size_) throw decoder_error("Empty data chunk!");
  }

  // make sure there is enough space in the buffer to accomodate nsamples
  buf_.resize(nsamples * header_.bytesPerSample, 0);

  // in case not enough samples are available to fulfill nsamples_
  if(buf_.size() > remaining_chunk_size_) {
    buf_.resize(remaining_chunk_size_, 0);
  }

  file_.read(&buf_[0], buf_.size());  // explictely allowed since C++11
  remaining_chunk_size_ -= file_.gcount();

  return buf_;
}
  
} // namespace vscharf

#ifdef TEST_WAV
// some basic unit testing
#include <cassert>
#include <iostream>
int main(int argc, char* argv[]) {
  vscharf::WavDecoder w("test_data/sound.wav");
  assert(w.get_header().channels == 1);
  assert(w.get_header().samplesPerSec == 44100);
  assert(w.get_header().avgBytesPerSec == 0x15888);
  assert(w.get_header().blockAlign == 0x2);
  assert(w.get_header().bitsPerSample == 16);

  std::size_t nsamples = 0;
  std::size_t nbytes = 0;

  const auto& buf = w.read_samples(1);
  ++nsamples;
  nbytes += buf.size();
 
  while(w.has_next()) {
    const auto& buf = w.read_samples(10);
    if(w.has_next()) assert(buf.size() == 20);
    nsamples += buf.size() / 2;
    nbytes += buf.size();
  }

  assert(nsamples == 0x10266 / 2);
  assert(nbytes == 0x10266);

  std::cout << "Test finished successfully!" << std::endl;
}
#endif // TEST_WAV
