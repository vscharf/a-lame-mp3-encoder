CXXFLAGS = -Iinclude -std=c++11 -g

default: bin/a-lame-mp3-encoder

.PHONY:
tests: dirs bin/wav_test bin/dir_test bin/enc_test

.PHONY:
clean:
	@rm -f bin/wav_test bin/dir_test bin_enc_test

dirs:
	@mkdir -p bin

bin/wav_test: src/wavdecoder.cpp
	@$(CXX) -DTEST_WAV $(CXXFLAGS) $(CPPFLAGS) -o $@ $^

bin/dir_test: src/directory.cpp
	@$(CXX) -DTEST_DIR $(CXXFLAGS) $(CPPFLAGS) -o $@ $^

bin/enc_test: src/mp3encoder.cpp src/wavdecoder.cpp
	@$(CXX) -DTEST_ENCODER $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ -I/usr/include/lame -lmp3lame

bin/a-lame-mp3-encoder: src/mp3encoder.cpp src/wavdecoder.cpp src/directory.cpp src/batch-encoder.cpp
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ -I/usr/include/lame -lmp3lame -pthread
