// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// register components
//
// 2011-06-30 GONG Chen <chen.sst@gmail.com>
//

#include <rime/mapped_file.h>

namespace rime {

MappedFile::MappedFile(const std::string &file_name)
    : file_name_(file_name) {
}

MappedFile::~MappedFile() {
  if (file_) {
    // TODO:
  }
}
  
bool MappedFile::Create(size_t size) {
  return false;
}

bool MappedFile::OpenOrCreate(size_t size) {
  return false;
}

bool MappedFile::OpenReadOnly() {
  return false;
}

void MappedFile::Close() {
  
}

bool MappedFile::IsOpen() const {
  return false;
}

bool MappedFile::Grow(size_t size) {
  return false;
}

bool MappedFile::ShrinkToFit() {
  return false;
}

}  // namespace rime
