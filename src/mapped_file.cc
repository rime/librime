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
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <rime/mapped_file.h>

using boost::interprocess::file_mapping;
using boost::interprocess::managed_mapped_file;
using boost::interprocess::create_only;
using boost::interprocess::open_or_create;
using boost::interprocess::open_only;

namespace rime {

MappedFile::MappedFile(const std::string &file_name)
    : file_name_(file_name) {
}

MappedFile::~MappedFile() {
  if (file_) {
    file_.reset();
  }
}
  
bool MappedFile::Create(size_t size) {
  file_.reset(new managed_mapped_file(create_only, file_name_.c_str(), size));
  return file_;
}

bool MappedFile::OpenOrCreate(size_t size) {
  file_.reset(new managed_mapped_file(open_or_create, file_name_.c_str(), size));
  return file_;
}

bool MappedFile::OpenReadOnly() {
  file_.reset(new managed_mapped_file(open_only, file_name_.c_str()));
  return file_;
}

void MappedFile::Close() {
  if (file_) {
    file_.reset();
  }
}

bool MappedFile::IsOpen() const {
  return file_;
}

bool MappedFile::Grow(size_t size) {
  if (IsOpen())
    return false;
  return managed_mapped_file::grow(file_name_.c_str(), size);
}

bool MappedFile::ShrinkToFit() {
  if (IsOpen())
    return false;
  return managed_mapped_file::shrink_to_fit(file_name_.c_str());
}

bool MappedFile::Remove() {
  return file_mapping::remove(file_name_.c_str());
}

}  // namespace rime
