// Copyright (c) Google LLC 2019
//
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <cstdio>
#include <cstdlib>
#include <string>

#include "../common/jpeg_data.h"
#include "../common/types.h"
#include "../enc/brunsli_encode.h"
#include "../enc/jpeg_data_reader.h"

bool ReadFileInternal(FILE* file, std::string* content) {
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "Failed to seek end of input file.\n");
    return false;
  }
  int input_size = ftell(file);
  if (input_size == 0) {
    fprintf(stderr, "Input file is empty.\n");
    return false;
  }
  if (fseek(file, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Failed to rewind input file to the beginning.\n");
    return false;
  }
  content->resize(input_size);
  size_t read_pos = 0;
  while (read_pos < content->size()) {
    const size_t bytes_read =
        fread(&content->at(read_pos), 1, content->size() - read_pos, file);
    if (bytes_read == 0) {
      fprintf(stderr, "Failed to read input file\n");
      return false;
    }
    read_pos += bytes_read;
  }
  return true;
}

bool ReadFile(const std::string& file_name, std::string* content) {
  FILE* file = fopen(file_name.c_str(), "rb");
  if (file == nullptr) {
    fprintf(stderr, "Failed to open input file.\n");
    return false;
  }
  bool ok = ReadFileInternal(file, content);
  if (fclose(file) != 0) {
    if (ok) {
      fprintf(stderr, "Failed to close input file.\n");
    }
    return false;
  }
  return ok;
}

bool WriteFileInternal(FILE* file, const std::string& content) {
  size_t write_pos = 0;
  while (write_pos < content.size()) {
    const size_t bytes_written =
        fwrite(&content[write_pos], 1, content.size() - write_pos, file);
    if (bytes_written == 0) {
      fprintf(stderr, "Failed to write output.\n");
      return false;
    }
    write_pos += bytes_written;
  }
  return true;
}

bool WriteFile(const std::string& file_name, const std::string& content) {
  FILE* file = fopen(file_name.c_str(), "wb");
  if (file == nullptr) {
    fprintf(stderr, "Failed to open file for writing.\n");
    return false;
  }
  bool ok = WriteFileInternal(file, content);
  if (fclose(file) != 0) {
    if (ok) {
      fprintf(stderr, "Failed to close output file.\n");
    }
    return false;
  }
  return ok;
}

bool ProcessFile(const std::string& file_name) {
  std::string input;
  if (!ReadFile(file_name, &input)) {
    return false;
  }

  std::string output;
  {
    brunsli::JPEGData jpg;
    const uint8_t* input_data = reinterpret_cast<const uint8_t*>(input.data());
    bool ok = brunsli::ReadJpeg(input_data, input.size(),
                                brunsli::JPEG_READ_ALL, &jpg);
    input.clear();
    input.shrink_to_fit();
    if (!ok) {
      fprintf(stderr, "Failed to parse JPEG input.\n");
      return false;
    }
    size_t output_size = brunsli::GetMaximumBrunsliEncodedSize(jpg);
    output.resize(output_size);
    uint8_t* output_data = reinterpret_cast<uint8_t*>(&output[0]);
    ok = brunsli::BrunsliEncodeJpeg(jpg, true, output_data, &output_size);
    if (!ok) {
      // TODO: use fallback?
      fprintf(stderr, "Failed to transform JPEG to Brunsli\n");
      return false;
    }
    output.resize(output_size);
  }

  return WriteFile(file_name + ".brn", output);
}

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: cbrunsli FILE\n");
    return EXIT_FAILURE;
  }
  std::string file_name = std::string(argv[1]);
  if (file_name.empty()) {
    fprintf(stderr, "Empty input file name.\n");
    return EXIT_FAILURE;
  }
  return ProcessFile(file_name) ? EXIT_SUCCESS : EXIT_FAILURE;
}
