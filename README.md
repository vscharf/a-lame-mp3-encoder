# a-lame-mp3-encoder
Given a directory of .wav files encode them to .mp3 files leveraging parallelism.

## Requirements
1. application is called with pathname as argument, e.g. `<applicationname> F:\MyWavCollection` all WAV-files contained directly in that folder are to be encoded to MP3
2. use all available CPU cores for the encoding process in an efficient way by utilizing multi-threading
3. statically link to lame encoder library
4. application should be compilable and runnable on Windows and Linux
5. the resulting MP3 files are to be placed within the same directory as the source WAV files, the filename extension should be changed appropriately to .MP3
6. non-WAV files in the given folder shall be ignored
7. multithreading shall be implemented by means of using Posix Threads (there exist implementations for Windows)
8. the Boost library shall not be used
9. the LAME encoder should be used with reasonable standard settings (e.g. quality based encoding with quality level "good")

## Decision Rational
2. Given the additional information that typically a large number (> 100) of WAV-files will be converted, I decided to use per-file concurrency. In a scenario where a small number of large WAV-files has to be converted a per-chunk concurrency would be more suited. This however is a bit more difficult to implement. One very typical solution would be to use a pipeline, e.g. from Intel's TBB libraries.

4. The code has been setup to be able to compile on Windows and Linux. Unfortunately I don't own a Windows Licences and couldn't test the resulting code. Keeping my fingers crossed ...

## Files
The converted uses 4 different modules all in namespace `vscharf`:
* directory: Wraps the directory traversal behind a single function to hide the additional complexity from platform dependence.
* wavdecoder: Reads a WAV-file, decodes the header and provider the sample data.
* mp3encoder: Retrieves input from a `WavDecoder` and encodes it to mp3 format using the lame library.
* pthread_wrapper: Header-only module that wraps the POSIX pthread calls to add RAII.

## Compiling
Compilation is done using cmake. The only option to be given is the include directory of liblame, i.e. the directory that contains `lame.h`.

## Binaries
Successful compilation will produce four binaries, three test and the actual encoder. All tests should finish successfully when start from the root directory of the project, i.e. the directory that contains the CMakeLists.txt.

