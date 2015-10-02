#include "helper.h"

#ifdef WINDOWS
#else
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#endif

namespace vscharf {

const char* posix_error::what() const noexcept
{
  return strerror(_err);
}

namespace {
#ifdef WINDOWS
#else
class scoped_dir { // RAII for opendir/closedir
public:
  scoped_dir(const std::string& path) : _dirp(opendir(path.c_str())) {
    if(!_dirp) throw posix_error(errno);
  }
  ~scoped_dir() {
    closedir(_dirp); // ignore error as destructor must never fail
  }
  DIR* dir() const { return _dirp; }
private:
  DIR* _dirp;
}; // class scoped_dir
#endif
} // anonymous namespace

std::vector<std::string> directory_entries(const std::string& path)
{
  std::vector<std::string> entries;
  scoped_dir dir(path);
#ifdef WINDOWS
#else
  errno = 0;
  dirent* current = nullptr;
  while((current = readdir(dir.dir())) != nullptr) {
    entries.emplace_back(current->d_name);
  }
  if(errno) throw posix_error(errno);
#endif
  return entries; // rely on copy ellision / move
} // directory_entries
  
} // namespace vscharf


#ifdef TEST
#include <algorithm> // std::equal
#include <cassert>
int main()
{
  using std::begin;
  using std::end;
  {
    // assumes the test is called in the project root directory
    std::vector<std::string> expected = {".", "..", "empty_dir", "file1", "file2", "file3"};
    try {
      std::vector<std::string> actual = vscharf::directory_entries("test_data");
      std::sort(begin(actual), end(actual));
      assert(std::equal(begin(expected), end(expected), begin(actual)));
    } catch(...) {
      assert(false && "Exception thrown");
    }
  }

  {
    // assumes the test is called in the project root directory
    std::vector<std::string> expected = {".", ".."};
    try {
      std::vector<std::string> actual = vscharf::directory_entries("test_data/empty_dir");
      std::sort(begin(actual), end(actual));
      assert(std::equal(begin(expected), end(expected), begin(actual)));
    } catch(...) {
      assert(false && "Exception thrown");
    }
  }

  {
    // assumes the test is called in the project root directory
    try {
      std::vector<std::string> actual = vscharf::directory_entries("non_existent_dir");
    } catch(const vscharf::posix_error&) {
      return 0;
    }
    assert(false && "Expected exception");
  }
  
  // not reachable 
  return 0;
} // main
#endif
