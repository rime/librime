#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif  // _MSC_VER

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sstream>

#include <marisa/grimoire/io.h>

#include "marisa-assert.h"

namespace {

void TestFilename() {
  TEST_START();

  {
    marisa::grimoire::Writer writer;
    writer.open("io-test.dat");

    writer.write((marisa::UInt32)123);
    writer.write((marisa::UInt32)234);

    double values[] = { 3.45, 4.56 };
    writer.write(values, 2);

    EXCEPT(writer.write(values, MARISA_SIZE_MAX), MARISA_SIZE_ERROR);
  }

  {
    marisa::grimoire::Reader reader;
    reader.open("io-test.dat");

    marisa::UInt32 value;
    reader.read(&value);
    ASSERT(value == 123);
    reader.read(&value);
    ASSERT(value == 234);

    double values[2];
    reader.read(values, 2);
    ASSERT(values[0] == 3.45);
    ASSERT(values[1] == 4.56);

    char byte;
    EXCEPT(reader.read(&byte), MARISA_IO_ERROR);
  }

  {
    marisa::grimoire::Mapper mapper;
    mapper.open("io-test.dat");

    marisa::UInt32 value;
    mapper.map(&value);
    ASSERT(value == 123);
    mapper.map(&value);
    ASSERT(value == 234);

    const double *values;
    mapper.map(&values, 2);
    ASSERT(values[0] == 3.45);
    ASSERT(values[1] == 4.56);

    char byte;
    EXCEPT(mapper.map(&byte), MARISA_IO_ERROR);
  }

  {
    marisa::grimoire::Writer writer;
    writer.open("io-test.dat");
  }

  {
    marisa::grimoire::Reader reader;
    reader.open("io-test.dat");

    char byte;
    EXCEPT(reader.read(&byte), MARISA_IO_ERROR);
  }

  TEST_END();
}

void TestFd() {
  TEST_START();

  {
#ifdef _MSC_VER
    int fd = -1;
    ASSERT(::_sopen_s(&fd, "io-test.dat",
        _O_BINARY | _O_CREAT | _O_WRONLY | _O_TRUNC,
        _SH_DENYRW, _S_IREAD | _S_IWRITE) == 0);
#else  // _MSC_VER
    int fd = ::creat("io-test.dat", 0644);
    ASSERT(fd != -1);
#endif  // _MSC_VER
    marisa::grimoire::Writer writer;
    writer.open(fd);

    marisa::UInt32 value = 234;
    writer.write(value);

    double values[] = { 34.5, 67.8 };
    writer.write(values, 2);

#ifdef _MSC_VER
    ASSERT(::_close(fd) == 0);
#else  // _MSC_VER
    ASSERT(::close(fd) == 0);
#endif  // _MSC_VER
  }

  {
#ifdef _MSC_VER
    int fd = -1;
    ASSERT(::_sopen_s(&fd, "io-test.dat", _O_BINARY | _O_RDONLY,
        _SH_DENYRW, _S_IREAD) == 0);
#else  // _MSC_VER
    int fd = ::open("io-test.dat", O_RDONLY);
    ASSERT(fd != -1);
#endif  // _MSC_VER
    marisa::grimoire::Reader reader;
    reader.open(fd);

    marisa::UInt32 value;
    reader.read(&value);
    ASSERT(value == 234);

    double values[2];
    reader.read(values, 2);
    ASSERT(values[0] == 34.5);
    ASSERT(values[1] == 67.8);

    char byte;
    EXCEPT(reader.read(&byte), MARISA_IO_ERROR);

#ifdef _MSC_VER
    ASSERT(::_close(fd) == 0);
#else  // _MSC_VER
    ASSERT(::close(fd) == 0);
#endif  // _MSC_VER
  }

  TEST_END();
}

void TestFile() {
  TEST_START();

  {
#ifdef _MSC_VER
    FILE *file = NULL;
    ASSERT(::fopen_s(&file, "io-test.dat", "wb") == 0);
#else  // _MSC_VER
    FILE *file = std::fopen("io-test.dat", "wb");
    ASSERT(file != NULL);
#endif  // _MSC_VER
    marisa::grimoire::Writer writer;
    writer.open(file);

    marisa::UInt32 value = 10;
    writer.write(value);

    double values[2] = { 0.1, 0.2 };
    writer.write(values, 2);

    ASSERT(std::fclose(file) == 0);
  }

  {
#ifdef _MSC_VER
    FILE *file = NULL;
    ASSERT(::fopen_s(&file, "io-test.dat", "rb") == 0);
#else  // _MSC_VER
    FILE *file = std::fopen("io-test.dat", "rb");
    ASSERT(file != NULL);
#endif  // _MSC_VER
    marisa::grimoire::Reader reader;
    reader.open(file);

    marisa::UInt32 value;
    reader.read(&value);
    ASSERT(value == 10);

    double values[2];
    reader.read(values, 2);
    ASSERT(values[0] == 0.1);
    ASSERT(values[1] == 0.2);

    char byte;
    EXCEPT(reader.read(&byte), MARISA_IO_ERROR);

    ASSERT(std::fclose(file) == 0);
  }

  TEST_END();
}

void TestStream() {
  TEST_START();

  std::stringstream stream;

  {
    marisa::grimoire::Writer writer;
    writer.open(stream);

    marisa::UInt32 value = 12;
    writer.write(value);

    double values[2] = { 3.4, 5.6 };
    writer.write(values, 2);
  }

  {
    marisa::grimoire::Reader reader;
    reader.open(stream);

    marisa::UInt32 value;
    reader.read(&value);
    ASSERT(value == 12);

    double values[2];
    reader.read(values, 2);
    ASSERT(values[0] == 3.4);
    ASSERT(values[1] == 5.6);

    char byte;
    EXCEPT(reader.read(&byte), MARISA_IO_ERROR);
  }

  TEST_END();
}

}  // namespace

int main() try {
  TestFilename();
  TestFd();
  TestFile();
  TestStream();

  return 0;
} catch (const marisa::Exception &ex) {
  std::cerr << ex.what() << std::endl;
  throw;
}
