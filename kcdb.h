/*************************************************************************************************
 * Database interface
 *                                                               Copyright (C) 2009-2012 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#ifndef _KCDB_H                          // duplication check
#define _KCDB_H

#include <kccommon.h>
#include <kcutil.h>
#include <kcthread.h>
#include <kcfile.h>
#include <kccompress.h>
#include <kccompare.h>
#include <kcmap.h>

#define KCDBSSMAGICDATA  "KCSS\n"        ///< The magic data of the snapshot file

namespace kyotocabinet {                 // common namespace


/**
 * Interface of database abstraction.
 * @note This class is an abstract class to prescribe the interface of record access.
 */
class DB {
 public:
  /**
   * Interface to access a record.
   */
  class Visitor {
   public:
    /** Special pointer for no operation. */
    static const char* const NOP;
    /** Special pointer to remove the record. */
    static const char* const REMOVE;
    /**
     * Destructor.
     */
    virtual ~Visitor() {
      _assert_(true);
    }
    /**
     * Visit a record.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @param vbuf the pointer to the value region.
     * @param vsiz the size of the value region.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @return If it is the pointer to a region, the value is replaced by the content.  If it
     * is Visitor::NOP, nothing is modified.  If it is Visitor::REMOVE, the record is removed.
     */
    virtual const char* visit_full(const char* kbuf, size_t ksiz,
                                   const char* vbuf, size_t vsiz, size_t* sp) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ && sp);
      return NOP;
    }
    /**
     * Visit a empty record space.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @return If it is the pointer to a region, the value is replaced by the content.  If it
     * is Visitor::NOP or Visitor::REMOVE, nothing is modified.
     */
    virtual const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ && sp);
      return NOP;
    }
    /**
     * Preprocess the main operations.
     */
    virtual void visit_before() {
      _assert_(true);
    }
    /**
     * Postprocess the main operations.
     */
    virtual void visit_after() {
      _assert_(true);
    }
  };
  /**
   * Interface of cursor to indicate a record.
   */
  class Cursor {
   public:
    /**
     * Destructor.
     */
    virtual ~Cursor() {
      _assert_(true);
    }
    /**
     * Accept a visitor to the current record.
     * @param visitor a visitor object.
     * @param writable true for writable operation, or false for read-only operation.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return true on success, or false on failure.
     * @note The operation for each record is performed atomically and other threads accessing
     * the same record are blocked.  To avoid deadlock, any explicit database operation must not
     * be performed in this function.
     */
    virtual bool accept(Visitor* visitor, bool writable = true, bool step = false) = 0;
    /**
     * Set the value of the current record.
     * @param vbuf the pointer to the value region.
     * @param vsiz the size of the value region.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return true on success, or false on failure.
     */
    virtual bool set_value(const char* vbuf, size_t vsiz, bool step = false) = 0;
    /**
     * Set the value of the current record.
     * @note Equal to the original Cursor::set_value method except that the parameter is
     * std::string.
     */
    virtual bool set_value_str(const std::string& value, bool step = false) = 0;
    /**
     * Remove the current record.
     * @return true on success, or false on failure.
     * @note If no record corresponds to the key, false is returned.  The cursor is moved to the
     * next record implicitly.
     */
    virtual bool remove() = 0;
    /**
     * Get the key of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return the pointer to the key region of the current record, or NULL on failure.
     * @note If the cursor is invalidated, NULL is returned.  Because an additional zero
     * code is appended at the end of the region of the return value, the return value can be
     * treated as a C-style string.  Because the region of the return value is allocated with the
     * the new[] operator, it should be released with the delete[] operator when it is no longer
     * in use.
     */
    virtual char* get_key(size_t* sp, bool step = false) = 0;
    /**
     * Get the key of the current record.
     * @note Equal to the original Cursor::get_key method except that a parameter is a string to
     * contain the result and the return value is bool for success.
     */
    virtual bool get_key(std::string* key, bool step = false) = 0;
    /**
     * Get the value of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return the pointer to the value region of the current record, or NULL on failure.
     * @note If the cursor is invalidated, NULL is returned.  Because an additional zero
     * code is appended at the end of the region of the return value, the return value can be
     * treated as a C-style string.  Because the region of the return value is allocated with the
     * the new[] operator, it should be released with the delete[] operator when it is no longer
     * in use.
     */
    virtual char* get_value(size_t* sp, bool step = false) = 0;
    /**
     * Get the value of the current record.
     * @note Equal to the original Cursor::get_value method except that a parameter is a string
     * to contain the result and the return value is bool for success.
     */
    virtual bool get_value(std::string* value, bool step = false) = 0;
    /**
     * Get a pair of the key and the value of the current record.
     * @param ksp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param vbp the pointer to the variable into which the pointer to the value region is
     * assigned.
     * @param vsp the pointer to the variable into which the size of the value region is
     * assigned.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return the pointer to the key region, or NULL on failure.
     * @note If the cursor is invalidated, NULL is returned.  Because an additional zero code is
     * appended at the end of each region of the key and the value, each region can be treated
     * as a C-style string.  The return value should be deleted explicitly by the caller with
     * the detele[] operator.
     */
    virtual char* get(size_t* ksp, const char** vbp, size_t* vsp, bool step = false) = 0;
    /**
     * Get a pair of the key and the value of the current record.
     * @note Equal to the original Cursor::get method except that parameters are strings
     * to contain the result and the return value is bool for success.
     */
    virtual bool get(std::string* key, std::string* value, bool step = false) = 0;
    /**
     * Jump the cursor to the first record for forward scan.
     * @return true on success, or false on failure.
     */
    virtual bool jump() = 0;
    /**
     * Jump the cursor to a record for forward scan.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @return true on success, or false on failure.
     */
    virtual bool jump(const char* kbuf, size_t ksiz) = 0;
    /**
     * Jump the cursor to a record for forward scan.
     * @note Equal to the original Cursor::jump method except that the parameter is std::string.
     */
    virtual bool jump(const std::string& key) = 0;
    /**
     * Jump the cursor to the last record for backward scan.
     * @return true on success, or false on failure.
     * @note This method is dedicated to tree databases.  Some database types, especially hash
     * databases, will provide a dummy implementation.
     */
    virtual bool jump_back() = 0;
    /**
     * Jump the cursor to a record for backward scan.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @return true on success, or false on failure.
     * @note This method is dedicated to tree databases.  Some database types, especially hash
     * databases, will provide a dummy implementation.
     */
    virtual bool jump_back(const char* kbuf, size_t ksiz) = 0;
    /**
     * Jump the cursor to a record for backward scan.
     * @note Equal to the original Cursor::jump_back method except that the parameter is
     * std::string.
     */
    virtual bool jump_back(const std::string& key) = 0;
    /**
     * Step the cursor to the next record.
     * @return true on success, or false on failure.
     */
    virtual bool step() = 0;
    /**
     * Step the cursor to the previous record.
     * @return true on success, or false on failure.
     * @note This method is dedicated to tree databases.  Some database types, especially hash
     * databases, will provide a dummy implementation.
     */
    virtual bool step_back() = 0;
    /**
     * Get the database object.
     * @return the database object.
     */
    virtual DB* db() = 0;
  };
  /**
   * Destructor.
   */
  virtual ~DB() {
    _assert_(true);
  }
  /**
   * Accept a visitor to a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @return true on success, or false on failure.
   * @note The operation for each record is performed atomically and other threads accessing the
   * same record are blocked.  To avoid deadlock, any explicit database operation must not be
   * performed in this function.
   */
  virtual bool accept(const char* kbuf, size_t ksiz, Visitor* visitor, bool writable = true) = 0;
  /**
   * Set the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the value is overwritten.
   */
  virtual bool set(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) = 0;
  /**
   * Set the value of a record.
   * @note Equal to the original DB::set method except that the parameters are std::string.
   */
  virtual bool set(const std::string& key, const std::string& value) = 0;
  /**
   * Add a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the record is not modified and false is returned.
   */
  virtual bool add(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) = 0;
  /**
   * Set the value of a record.
   * @note Equal to the original DB::add method except that the parameters are std::string.
   */
  virtual bool add(const std::string& key, const std::string& value) = 0;
  /**
   * Replace the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, no new record is created and false is returned.
   * If the corresponding record exists, the value is modified.
   */
  virtual bool replace(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) = 0;
  /**
   * Replace the value of a record.
   * @note Equal to the original DB::replace method except that the parameters are std::string.
   */
  virtual bool replace(const std::string& key, const std::string& value) = 0;
  /**
   * Append the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the given value is appended at the end of the existing value.
   */
  virtual bool append(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) = 0;
  /**
   * Set the value of a record.
   * @note Equal to the original DB::append method except that the parameters are std::string.
   */
  virtual bool append(const std::string& key, const std::string& value) = 0;
  /**
   * Add a number to the numeric integer value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param num the additional number.
   * @param orig the origin number if no record corresponds to the key.  If it is INT64MIN and
   * no record corresponds, this function fails.  If it is INT64MAX, the value is set as the
   * additional number regardless of the current value.
   * @return the result value, or kyotocabinet::INT64MIN on failure.
   * @note The value is serialized as an 8-byte binary integer in big-endian order, not a decimal
   * string.  If existing value is not 8-byte, this function fails.
   */
  virtual int64_t increment(const char* kbuf, size_t ksiz, int64_t num, int64_t orig = 0) = 0;
  /**
   * Add a number to the numeric integer value of a record.
   * @note Equal to the original DB::increment method except that the parameter is std::string.
   */
  virtual int64_t increment(const std::string& key, int64_t num, int64_t orig = 0) = 0;
  /**
   * Add a number to the numeric double value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param num the additional number.
   * @param orig the origin number if no record corresponds to the key.  If it is negative
   * infinity and no record corresponds, this function fails.  If it is positive infinity, the
   * value is set as the additional number regardless of the current value.
   * @return the result value, or Not-a-number on failure.
   * @note The value is serialized as an 16-byte binary fixed-point number in big-endian order,
   * not a decimal string.  If existing value is not 16-byte, this function fails.
   */
  virtual double increment_double(const char* kbuf, size_t ksiz, double num,
                                  double orig = 0) = 0;
  /**
   * Add a number to the numeric double value of a record.
   * @note Equal to the original DB::increment_double method except that the parameter is
   * std::string.
   */
  virtual double increment_double(const std::string& key, double num, double orig = 0) = 0;
  /**
   * Perform compare-and-swap.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param ovbuf the pointer to the old value region.  NULL means that no record corresponds.
   * @param ovsiz the size of the old value region.
   * @param nvbuf the pointer to the new value region.  NULL means that the record is removed.
   * @param nvsiz the size of new old value region.
   * @return true on success, or false on failure.
   */
  virtual bool cas(const char* kbuf, size_t ksiz,
                   const char* ovbuf, size_t ovsiz, const char* nvbuf, size_t nvsiz) = 0;
  /**
   * Perform compare-and-swap.
   * @note Equal to the original DB::cas method except that the parameters are std::string.
   */
  virtual bool cas(const std::string& key,
                   const std::string& ovalue, const std::string& nvalue) = 0;
  /**
   * Remove a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, false is returned.
   */
  virtual bool remove(const char* kbuf, size_t ksiz) = 0;
  /**
   * Remove a record.
   * @note Equal to the original DB::remove method except that the parameter is std::string.
   */
  virtual bool remove(const std::string& key) = 0;
  /**
   * Retrieve the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @return the pointer to the value region of the corresponding record, or NULL on failure.
   * @note If no record corresponds to the key, NULL is returned.  Because an additional zero
   * code is appended at the end of the region of the return value, the return value can be
   * treated as a C-style string.  Because the region of the return value is allocated with the
   * the new[] operator, it should be released with the delete[] operator when it is no longer
   * in use.
   */
  virtual char* get(const char* kbuf, size_t ksiz, size_t* sp) = 0;
  /**
   * Retrieve the value of a record.
   * @note Equal to the original DB::get method except that the first parameters is the key
   * string and the second parameter is a string to contain the result and the return value is
   * bool for success.
   */
  virtual bool get(const std::string& key, std::string* value) = 0;
  /**
   * Retrieve the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the buffer into which the value of the corresponding record is
   * written.
   * @param max the size of the buffer.
   * @return the size of the value, or -1 on failure.
   */
  virtual int32_t get(const char* kbuf, size_t ksiz, char* vbuf, size_t max) = 0;
  /**
   * Check the existence of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @return the size of the value, or -1 on failure.
   */
  virtual int32_t check(const char* kbuf, size_t ksiz) = 0;
  /**
   * Check the existence of a record.
   * @note Equal to the original DB::check method except that the parameter is std::string.
   */
  virtual int32_t check(const std::string& key) = 0;
  /**
   * Remove all records.
   * @return true on success, or false on failure.
   */
  virtual bool clear() = 0;
  /**
   * Get the number of records.
   * @return the number of records, or -1 on failure.
   */
  virtual int64_t count() = 0;
  /**
   * Create a cursor object.
   * @return the return value is the created cursor object.
   * @note Because the object of the return value is allocated by the constructor, it should be
   * released with the delete operator when it is no longer in use.
   */
  virtual Cursor* cursor() = 0;
};


/**
 * Basic implementation of database.
 * @note This class is an abstract class to prescribe the interface of file operations and
 * provide mix-in methods.  This class can be inherited but overwriting methods is forbidden.
 * Before every database operation, it is necessary to call the BasicDB::open method in order to
 * open a database file and connect the database object to it.  To avoid data missing or
 * corruption, it is important to close every database file by the BasicDB::close method when the
 * database is no longer in use.  It is forbidden for multible database objects in a process to
 * open the same database at the same time.  It is forbidden to share a database object with
 * child processes.
 */
class BasicDB : public DB {
 public:
  class Cursor;
  class Error;
  class ProgressChecker;
  class FileProcessor;
  class Logger;
  class MetaTrigger;
 private:
  /** The size of the IO buffer. */
  static const size_t IOBUFSIZ = 8192;
 public:
  /**
   * Database types.
   */
  enum Type {
    TYPEVOID = 0x00,                     ///< void database
    TYPEPHASH = 0x10,                    ///< prototype hash database
    TYPEPTREE = 0x11,                    ///< prototype tree database
    TYPESTASH = 0x18,                    ///< stash database
    TYPECACHE = 0x20,                    ///< cache hash database
    TYPEGRASS = 0x21,                    ///< cache tree database
    TYPEHASH = 0x30,                     ///< file hash database
    TYPETREE = 0x31,                     ///< file tree database
    TYPEDIR = 0x40,                      ///< directory hash database
    TYPEFOREST = 0x41,                   ///< directory tree database
    TYPETEXT = 0x50,                     ///< plain text database
    TYPEMISC = 0x80                      ///< miscellaneous database
  };
  /**
   * Interface of cursor to indicate a record.
   */
  class Cursor : public DB::Cursor {
   public:
    /**
     * Destructor.
     */
    virtual ~Cursor() {
      _assert_(true);
    }
    /**
     * Set the value of the current record.
     * @param vbuf the pointer to the value region.
     * @param vsiz the size of the value region.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return true on success, or false on failure.
     */
    bool set_value(const char* vbuf, size_t vsiz, bool step = false) {
      _assert_(vbuf && vsiz <= MEMMAXSIZ);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl(const char* vbuf, size_t vsiz) :
            vbuf_(vbuf), vsiz_(vsiz), ok_(false) {}
        bool ok() const {
          return ok_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          ok_ = true;
          *sp = vsiz_;
          return vbuf_;
        }
        const char* vbuf_;
        size_t vsiz_;
        bool ok_;
      };
      VisitorImpl visitor(vbuf, vsiz);
      if (!accept(&visitor, true, step)) return false;
      if (!visitor.ok()) return false;
      return true;
    }
    /**
     * Set the value of the current record.
     * @note Equal to the original Cursor::set_value method except that the parameter is
     * std::string.
     */
    bool set_value_str(const std::string& value, bool step = false) {
      _assert_(true);
      return set_value(value.c_str(), value.size(), step);
    }
    /**
     * Remove the current record.
     * @return true on success, or false on failure.
     * @note If no record corresponds to the key, false is returned.  The cursor is moved to the
     * next record implicitly.
     */
    bool remove() {
      _assert_(true);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl() : ok_(false) {}
        bool ok() const {
          return ok_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          ok_ = true;
          return REMOVE;
        }
        bool ok_;
      };
      VisitorImpl visitor;
      if (!accept(&visitor, true, false)) return false;
      if (!visitor.ok()) return false;
      return true;
    }
    /**
     * Get the key of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return the pointer to the key region of the current record, or NULL on failure.
     * @note If the cursor is invalidated, NULL is returned.  Because an additional zero
     * code is appended at the end of the region of the return value, the return value can be
     * treated as a C-style string.  Because the region of the return value is allocated with the
     * the new[] operator, it should be released with the delete[] operator when it is no longer
     * in use.
     */
    char* get_key(size_t* sp, bool step = false) {
      _assert_(sp);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl() : kbuf_(NULL), ksiz_(0) {}
        char* pop(size_t* sp) {
          *sp = ksiz_;
          return kbuf_;
        }
        void clear() {
          delete[] kbuf_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          kbuf_ = new char[ksiz+1];
          std::memcpy(kbuf_, kbuf, ksiz);
          kbuf_[ksiz] = '\0';
          ksiz_ = ksiz;
          return NOP;
        }
        char* kbuf_;
        size_t ksiz_;
      };
      VisitorImpl visitor;
      if (!accept(&visitor, false, step)) {
        visitor.clear();
        *sp = 0;
        return NULL;
      }
      size_t ksiz;
      char* kbuf = visitor.pop(&ksiz);
      if (!kbuf) {
        *sp = 0;
        return NULL;
      }
      *sp = ksiz;
      return kbuf;
    }
    /**
     * Get the key of the current record.
     * @note Equal to the original Cursor::get_key method except that a parameter is a string to
     * contain the result and the return value is bool for success.
     */
    bool get_key(std::string* key, bool step = false) {
      _assert_(key);
      size_t ksiz;
      char* kbuf = get_key(&ksiz, step);
      if (!kbuf) return false;
      key->clear();
      key->append(kbuf, ksiz);
      delete[] kbuf;
      return true;
    }
    /**
     * Get the value of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return the pointer to the value region of the current record, or NULL on failure.
     * @note If the cursor is invalidated, NULL is returned.  Because an additional zero
     * code is appended at the end of the region of the return value, the return value can be
     * treated as a C-style string.  Because the region of the return value is allocated with the
     * the new[] operator, it should be released with the delete[] operator when it is no longer
     * in use.
     */
    char* get_value(size_t* sp, bool step = false) {
      _assert_(sp);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl() : vbuf_(NULL), vsiz_(0) {}
        char* pop(size_t* sp) {
          *sp = vsiz_;
          return vbuf_;
        }
        void clear() {
          delete[] vbuf_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          vbuf_ = new char[vsiz+1];
          std::memcpy(vbuf_, vbuf, vsiz);
          vbuf_[vsiz] = '\0';
          vsiz_ = vsiz;
          return NOP;
        }
        char* vbuf_;
        size_t vsiz_;
      };
      VisitorImpl visitor;
      if (!accept(&visitor, false, step)) {
        visitor.clear();
        *sp = 0;
        return NULL;
      }
      size_t vsiz;
      char* vbuf = visitor.pop(&vsiz);
      if (!vbuf) {
        *sp = 0;
        return NULL;
      }
      *sp = vsiz;
      return vbuf;
    }
    /**
     * Get the value of the current record.
     * @note Equal to the original Cursor::get_value method except that a parameter is a string
     * to contain the result and the return value is bool for success.
     */
    bool get_value(std::string* value, bool step = false) {
      _assert_(value);
      size_t vsiz;
      char* vbuf = get_value(&vsiz, step);
      if (!vbuf) return false;
      value->clear();
      value->append(vbuf, vsiz);
      delete[] vbuf;
      return true;
    }
    /**
     * Get a pair of the key and the value of the current record.
     * @param ksp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param vbp the pointer to the variable into which the pointer to the value region is
     * assigned.
     * @param vsp the pointer to the variable into which the size of the value region is
     * assigned.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return the pointer to the key region, or NULL on failure.
     * @note If the cursor is invalidated, NULL is returned.  Because an additional zero code is
     * appended at the end of each region of the key and the value, each region can be treated
     * as a C-style string.  The return value should be deleted explicitly by the caller with
     * the detele[] operator.
     */
    char* get(size_t* ksp, const char** vbp, size_t* vsp, bool step = false) {
      _assert_(ksp && vbp && vsp);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl() : kbuf_(NULL), ksiz_(0), vbuf_(NULL), vsiz_(0) {}
        char* pop(size_t* ksp, const char** vbp, size_t* vsp) {
          *ksp = ksiz_;
          *vbp = vbuf_;
          *vsp = vsiz_;
          return kbuf_;
        }
        void clear() {
          delete[] kbuf_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          size_t rsiz = ksiz + 1 + vsiz + 1;
          kbuf_ = new char[rsiz];
          std::memcpy(kbuf_, kbuf, ksiz);
          kbuf_[ksiz] = '\0';
          ksiz_ = ksiz;
          vbuf_ = kbuf_ + ksiz + 1;
          std::memcpy(vbuf_, vbuf, vsiz);
          vbuf_[vsiz] = '\0';
          vsiz_ = vsiz;
          return NOP;
        }
        char* kbuf_;
        size_t ksiz_;
        char* vbuf_;
        size_t vsiz_;
      };
      VisitorImpl visitor;
      if (!accept(&visitor, false, step)) {
        visitor.clear();
        *ksp = 0;
        *vbp = NULL;
        *vsp = 0;
        return NULL;
      }
      return visitor.pop(ksp, vbp, vsp);
    }
    /**
     * Get a pair of the key and the value of the current record.
     * @note Equal to the original Cursor::get method except that parameters are strings
     * to contain the result and the return value is bool for success.
     */
    bool get(std::string* key, std::string* value, bool step = false) {
      _assert_(key && value);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl(std::string* key, std::string* value) :
            key_(key), value_(value), ok_(false) {}
        bool ok() {
          return ok_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          key_->clear();
          key_->append(kbuf, ksiz);
          value_->clear();
          value_->append(vbuf, vsiz);
          ok_ = true;
          return NOP;
        }
        std::string* key_;
        std::string* value_;
        bool ok_;
      };
      VisitorImpl visitor(key, value);
      if (!accept(&visitor, false, step)) return false;
      return visitor.ok();
    }
    /**
     * Get a pair of the key and the value of the current record and remove it atomically.
     * @param ksp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param vbp the pointer to the variable into which the pointer to the value region is
     * assigned.
     * @param vsp the pointer to the variable into which the size of the value region is
     * assigned.
     * @return the pointer to the key region, or NULL on failure.
     * @note If the cursor is invalidated, NULL is returned.  Because an additional zero code is
     * appended at the end of each region of the key and the value, each region can be treated
     * as a C-style string.  The return value should be deleted explicitly by the caller with
     * the detele[] operator.  The cursor is moved to the next record implicitly.
     */
    char* seize(size_t* ksp, const char** vbp, size_t* vsp) {
      _assert_(ksp && vbp && vsp);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl() : kbuf_(NULL), ksiz_(0), vbuf_(NULL), vsiz_(0) {}
        char* pop(size_t* ksp, const char** vbp, size_t* vsp) {
          *ksp = ksiz_;
          *vbp = vbuf_;
          *vsp = vsiz_;
          return kbuf_;
        }
        void clear() {
          delete[] kbuf_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          size_t rsiz = ksiz + 1 + vsiz + 1;
          kbuf_ = new char[rsiz];
          std::memcpy(kbuf_, kbuf, ksiz);
          kbuf_[ksiz] = '\0';
          ksiz_ = ksiz;
          vbuf_ = kbuf_ + ksiz + 1;
          std::memcpy(vbuf_, vbuf, vsiz);
          vbuf_[vsiz] = '\0';
          vsiz_ = vsiz;
          return REMOVE;
        }
        char* kbuf_;
        size_t ksiz_;
        char* vbuf_;
        size_t vsiz_;
      };
      VisitorImpl visitor;
      if (!accept(&visitor, true, false)) {
        visitor.clear();
        *ksp = 0;
        *vbp = NULL;
        *vsp = 0;
        return NULL;
      }
      return visitor.pop(ksp, vbp, vsp);
    }
    /**
     * Get a pair of the key and the value of the current record and remove it atomically.
     * @note Equal to the original Cursor::seize method except that parameters are strings
     * to contain the result and the return value is bool for success.
     */
    bool seize(std::string* key, std::string* value) {
      _assert_(key && value);
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl(std::string* key, std::string* value) :
            key_(key), value_(value), ok_(false) {}
        bool ok() {
          return ok_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          key_->clear();
          key_->append(kbuf, ksiz);
          value_->clear();
          value_->append(vbuf, vsiz);
          ok_ = true;
          return REMOVE;
        }
        std::string* key_;
        std::string* value_;
        bool ok_;
      };
      VisitorImpl visitor(key, value);
      if (!accept(&visitor, true, false)) return false;
      return visitor.ok();
    }
    /**
     * Get the database object.
     * @return the database object.
     */
    virtual BasicDB* db() = 0;
    /**
     * Get the last happened error.
     * @return the last happened error.
     */
    Error error() {
      _assert_(true);
      return db()->error();
    }
  };
  /**
   * Error data.
   */
  class Error {
   public:
    /**
     * Error codes.
     */
    enum Code {
      SUCCESS,                           ///< success
      NOIMPL,                            ///< not implemented
      INVALID,                           ///< invalid operation
      NOREPOS,                           ///< no repository
      NOPERM,                            ///< no permission
      BROKEN,                            ///< broken file
      DUPREC,                            ///< record duplication
      NOREC,                             ///< no record
      LOGIC,                             ///< logical inconsistency
      SYSTEM,                            ///< system error
      MISC = 15                          ///< miscellaneous error
    };
    /**
     * Default constructor.
     */
    explicit Error() : code_(SUCCESS), message_("no error") {
      _assert_(true);
    }
    /**
     * Copy constructor.
     * @param src the source object.
     */
    Error(const Error& src) : code_(src.code_), message_(src.message_) {
      _assert_(true);
    }
    /**
     * Constructor.
     * @param code an error code.
     * @param message a supplement message.
     */
    explicit Error(Code code, const char* message) : code_(code), message_(message) {
      _assert_(message);
    }
    /**
     * Destructor.
     */
    ~Error() {
      _assert_(true);
    }
    /**
     * Set the error information.
     * @param code an error code.
     * @param message a supplement message.
     */
    void set(Code code, const char* message) {
      _assert_(message);
      code_ = code;
      message_ = message;
    }
    /**
     * Get the error code.
     * @return the error code.
     */
    Code code() const {
      _assert_(true);
      return code_;
    }
    /**
     * Get the readable string of the code.
     * @return the readable string of the code.
     */
    const char* name() const {
      _assert_(true);
      return codename(code_);
    }
    /**
     * Get the supplement message.
     * @return the supplement message.
     */
    const char* message() const {
      _assert_(true);
      return message_;
    }
    /**
     * Get the readable string of an error code.
     * @param code the error code.
     * @return the readable string of the error code.
     */
    static const char* codename(Code code) {
      _assert_(true);
      switch (code) {
        case SUCCESS: return "success";
        case NOIMPL: return "not implemented";
        case INVALID: return "invalid operation";
        case NOREPOS: return "no repository";
        case NOPERM: return "no permission";
        case BROKEN: return "broken file";
        case DUPREC: return "record duplication";
        case NOREC: return "no record";
        case LOGIC: return "logical inconsistency";
        case SYSTEM: return "system error";
        default: break;
      }
      return "miscellaneous error";
    }
    /**
     * Assignment operator from the self type.
     * @param right the right operand.
     * @return the reference to itself.
     */
    Error& operator =(const Error& right) {
      _assert_(true);
      if (&right == this) return *this;
      code_ = right.code_;
      message_ = right.message_;
      return *this;
    }
    /**
     * Cast operator to integer.
     * @return the error code.
     */
    operator int32_t() const {
      return code_;
    }
   private:
    /** The error code. */
    Code code_;
    /** The supplement message. */
    const char* message_;
  };
  /**
   * Interface to check progress status of long process.
   */
  class ProgressChecker {
   public:
    /**
     * Destructor.
     */
    virtual ~ProgressChecker() {
      _assert_(true);
    }
    /**
     * Check the progress status.
     * @param name the name of the process.
     * @param message a supplement message.
     * @param curcnt the count of the current step of the progress, or -1 if not applicable.
     * @param allcnt the estimation count of all steps of the progress, or -1 if not applicable.
     * @return true to continue the process, or false to stop the process.
     */
    virtual bool check(const char* name, const char* message,
                       int64_t curcnt, int64_t allcnt) = 0;
  };
  /**
   * Interface to process the database file.
   */
  class FileProcessor {
   public:
    /**
     * Destructor.
     */
    virtual ~FileProcessor() {
      _assert_(true);
    }
    /**
     * Process the database file.
     * @param path the path of the database file.
     * @param count the number of records.  A negative value means omission.
     * @param size the size of the available region.  A negative value means omission.
     * @return true on success, or false on failure.
     */
    virtual bool process(const std::string& path, int64_t count, int64_t size) = 0;
  };
  /**
   * Interface to log internal information and errors.
   */
  class Logger {
   public:
    /**
     * Event kinds.
     */
    enum Kind {
      DEBUG = 1 << 0,                    ///< debugging
      INFO = 1 << 1,                     ///< normal information
      WARN = 1 << 2,                     ///< warning
      ERROR = 1 << 3                     ///< error
    };
    /**
     * Destructor.
     */
    virtual ~Logger() {
      _assert_(true);
    }
    /**
     * Process a log message.
     * @param file the file name of the program source code.
     * @param line the line number of the program source code.
     * @param func the function name of the program source code.
     * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
     * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
     * @param message the supplement message.
     */
    virtual void log(const char* file, int32_t line, const char* func, Kind kind,
                     const char* message) = 0;
  };
  /**
   * Interface to trigger meta database operations.
   */
  class MetaTrigger {
   public:
    /**
     * Event kinds.
     */
    enum Kind {
      OPEN,                              ///< opening
      CLOSE,                             ///< closing
      CLEAR,                             ///< clearing
      ITERATE,                           ///< iteration
      SYNCHRONIZE,                       ///< synchronization
      OCCUPY,                            ///< occupation
      BEGINTRAN,                         ///< beginning transaction
      COMMITTRAN,                        ///< committing transaction
      ABORTTRAN,                         ///< aborting transaction
      MISC = 15                          ///< miscellaneous operation
    };
    /**
     * Destructor.
     */
    virtual ~MetaTrigger() {
      _assert_(true);
    }
    /**
     * Trigger a meta database operation.
     * @param kind the kind of the event.  MetaTrigger::OPEN for opening, MetaTrigger::CLOSE for
     * closing, MetaTrigger::CLEAR for clearing, MetaTrigger::ITERATE for iteration,
     * MetaTrigger::SYNCHRONIZE for synchronization, MetaTrigger::OCCUPY for occupation,
     * MetaTrigger::BEGINTRAN for beginning transaction, MetaTrigger::COMMITTRAN for committing
     * transaction, MetaTrigger::ABORTTRAN for aborting transaction, and MetaTrigger::MISC for
     * miscellaneous operations.
     * @param message the supplement message.
     */
    virtual void trigger(Kind kind, const char* message) = 0;
  };
  /**
   * Open modes.
   */
  enum OpenMode {
    OREADER = 1 << 0,                    ///< open as a reader
    OWRITER = 1 << 1,                    ///< open as a writer
    OCREATE = 1 << 2,                    ///< writer creating
    OTRUNCATE = 1 << 3,                  ///< writer truncating
    OAUTOTRAN = 1 << 4,                  ///< auto transaction
    OAUTOSYNC = 1 << 5,                  ///< auto synchronization
    ONOLOCK = 1 << 6,                    ///< open without locking
    OTRYLOCK = 1 << 7,                   ///< lock without blocking
    ONOREPAIR = 1 << 8                   ///< open without auto repair
  };
  /**
   * Destructor.
   * @note If the database is not closed, it is closed implicitly.
   */
  virtual ~BasicDB() {
    _assert_(true);
  }
  /**
   * Get the last happened error.
   * @return the last happened error.
   */
  virtual Error error() const = 0;
  /**
   * Set the error information.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param code an error code.
   * @param message a supplement message.
   */
  virtual void set_error(const char* file, int32_t line, const char* func,
                         Error::Code code, const char* message) = 0;
  /**
   * Open a database file.
   * @param path the path of a database file.
   * @param mode the connection mode.  BasicDB::OWRITER as a writer, BasicDB::OREADER as a
   * reader.  The following may be added to the writer mode by bitwise-or: BasicDB::OCREATE,
   * which means it creates a new database if the file does not exist, BasicDB::OTRUNCATE, which
   * means it creates a new database regardless if the file exists, BasicDB::OAUTOTRAN, which
   * means each updating operation is performed in implicit transaction, BasicDB::OAUTOSYNC,
   * which means each updating operation is followed by implicit synchronization with the file
   * system.  The following may be added to both of the reader mode and the writer mode by
   * bitwise-or: BasicDB::ONOLOCK, which means it opens the database file without file locking,
   * BasicDB::OTRYLOCK, which means locking is performed without blocking, File::ONOREPAIR, which
   * means the database file is not repaired implicitly even if file destruction is detected.
   * @return true on success, or false on failure.
   * @note Every opened database must be closed by the BasicDB::close method when it is no longer
   * in use.  It is not allowed for two or more database objects in the same process to keep
   * their connections to the same database file at the same time.
   */
  virtual bool open(const std::string& path, uint32_t mode = OWRITER | OCREATE) = 0;
  /**
   * Close the database file.
   * @return true on success, or false on failure.
   */
  virtual bool close() = 0;
  /**
   * Accept a visitor to multiple records at once.
   * @param keys specifies a string vector of the keys.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @return true on success, or false on failure.
   * @note The operations for specified records are performed atomically and other threads
   * accessing the same records are blocked.  To avoid deadlock, any explicit database operation
   * must not be performed in this function.
   */
  virtual bool accept_bulk(const std::vector<std::string>& keys, Visitor* visitor,
                           bool writable = true) = 0;
  /**
   * Iterate to accept a visitor for each record.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   * @note The whole iteration is performed atomically and other threads are blocked.  To avoid
   * deadlock, any explicit database operation must not be performed in this function.
   */
  virtual bool iterate(Visitor *visitor, bool writable = true,
                       ProgressChecker* checker = NULL) = 0;
  /**
   * Scan each record in parallel.
   * @param visitor a visitor object.
   * @param thnum the number of worker threads.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   * @note This function is for reading records and not for updating ones.  The return value of
   * the visitor is just ignored.  To avoid deadlock, any explicit database operation must not
   * be performed in this function.
   */
  virtual bool scan_parallel(Visitor *visitor, size_t thnum,
                             ProgressChecker* checker = NULL) = 0;
  /**
   * Synchronize updated contents with the file and the device.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @param proc a postprocessor object.  If it is NULL, no postprocessing is performed.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   * @note The operation of the postprocessor is performed atomically and other threads accessing
   * the same record are blocked.  To avoid deadlock, any explicit database operation must not
   * be performed in this function.
   */
  virtual bool synchronize(bool hard = false, FileProcessor* proc = NULL,
                           ProgressChecker* checker = NULL) = 0;
  /**
   * Occupy database by locking and do something meanwhile.
   * @param writable true to use writer lock, or false to use reader lock.
   * @param proc a processor object.  If it is NULL, no processing is performed.
   * @return true on success, or false on failure.
   * @note The operation of the processor is performed atomically and other threads accessing
   * the same record are blocked.  To avoid deadlock, any explicit database operation must not
   * be performed in this function.
   */
  virtual bool occupy(bool writable = true, FileProcessor* proc = NULL) = 0;
  /**
   * Create a copy of the database file.
   * @param dest the path of the destination file.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   */
  bool copy(const std::string& dest, ProgressChecker* checker = NULL) {
    _assert_(true);
    class FileProcessorImpl : public FileProcessor {
     public:
      explicit FileProcessorImpl(const std::string& dest, ProgressChecker* checker,
                                 BasicDB* db) :
          dest_(dest), checker_(checker), db_(db) {}
     private:
      bool process(const std::string& path, int64_t count, int64_t size) {
        File::Status sbuf;
        if (!File::status(path, &sbuf)) return false;
        if (sbuf.isdir) {
          if (!File::make_directory(dest_)) return false;
          bool err = false;
          DirStream dir;
          if (dir.open(path)) {
            if (checker_ && !checker_->check("copy", "beginning", 0, -1)) {
              db_->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
              err = true;
            }
            std::string name;
            int64_t curcnt = 0;
            while (!err && dir.read(&name)) {
              const std::string& spath = path + File::PATHCHR + name;
              const std::string& dpath = dest_ + File::PATHCHR + name;
              int64_t dsiz;
              char* dbuf = File::read_file(spath, &dsiz);
              if (dbuf) {
                if (!File::write_file(dpath, dbuf, dsiz)) err = true;
                delete[] dbuf;
              } else {
                err = true;
              }
              curcnt++;
              if (checker_ && !checker_->check("copy", "processing", curcnt, -1)) {
                db_->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
                err = true;
                break;
              }
            }
            if (checker_ && !checker_->check("copy", "ending", -1, -1)) {
              db_->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
              err = true;
            }
            if (!dir.close()) err = true;
          } else {
            err = true;
          }
          return !err;
        }
        std::ofstream ofs;
        ofs.open(dest_.c_str(),
                 std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        if (!ofs) return false;
        bool err = false;
        std::ifstream ifs;
        ifs.open(path.c_str(), std::ios_base::in | std::ios_base::binary);
        if (checker_ && !checker_->check("copy", "beginning", 0, size)) {
          db_->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
          err = true;
        }
        if (ifs) {
          char buf[IOBUFSIZ];
          int64_t curcnt = 0;
          while (!err && !ifs.eof()) {
            size_t n = ifs.read(buf, sizeof(buf)).gcount();
            if (n > 0) {
              ofs.write(buf, n);
              if (!ofs) {
                err = true;
                break;
              }
            }
            curcnt += n;
            if (checker_ && !checker_->check("copy", "processing", curcnt, size)) {
              db_->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
              err = true;
              break;
            }
          }
          ifs.close();
          if (ifs.bad()) err = true;
        } else {
          err = true;
        }
        if (checker_ && !checker_->check("copy", "ending", -1, size)) {
          db_->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
          err = true;
        }
        ofs.close();
        if (!ofs) err = true;
        return !err;
      }
      const std::string& dest_;
      ProgressChecker* checker_;
      BasicDB* db_;
    };
    FileProcessorImpl proc(dest, checker, this);
    return synchronize(false, &proc, checker);
  }
  /**
   * Begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  virtual bool begin_transaction(bool hard = false) = 0;
  /**
   * Try to begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  virtual bool begin_transaction_try(bool hard = false) = 0;
  /**
   * End transaction.
   * @param commit true to commit the transaction, or false to abort the transaction.
   * @return true on success, or false on failure.
   */
  virtual bool end_transaction(bool commit = true) = 0;
  /**
   * Get the size of the database file.
   * @return the size of the database file in bytes, or -1 on failure.
   */
  virtual int64_t size() = 0;
  /**
   * Get the path of the database file.
   * @return the path of the database file, or an empty string on failure.
   */
  virtual std::string path() = 0;
  /**
   * Get the miscellaneous status information.
   * @param strmap a string map to contain the result.
   * @return true on success, or false on failure.
   */
  virtual bool status(std::map<std::string, std::string>* strmap) = 0;
  /**
   * Set the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the value is overwritten.
   */
  bool set(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(const char* vbuf, size_t vsiz) : vbuf_(vbuf), vsiz_(vsiz) {}
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        *sp = vsiz_;
        return vbuf_;
      }
      const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
        *sp = vsiz_;
        return vbuf_;
      }
      const char* vbuf_;
      size_t vsiz_;
    };
    VisitorImpl visitor(vbuf, vsiz);
    if (!accept(kbuf, ksiz, &visitor, true)) return false;
    return true;
  }
  /**
   * Set the value of a record.
   * @note Equal to the original DB::set method except that the parameters are std::string.
   */
  bool set(const std::string& key, const std::string& value) {
    _assert_(true);
    return set(key.c_str(), key.size(), value.c_str(), value.size());
  }
  /**
   * Add a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the record is not modified and false is returned.
   */
  bool add(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(const char* vbuf, size_t vsiz) :
          vbuf_(vbuf), vsiz_(vsiz), ok_(false) {}
      bool ok() const {
        return ok_;
      }
     private:
      const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
        ok_ = true;
        *sp = vsiz_;
        return vbuf_;
      }
      const char* vbuf_;
      size_t vsiz_;
      bool ok_;
    };
    VisitorImpl visitor(vbuf, vsiz);
    if (!accept(kbuf, ksiz, &visitor, true)) return false;
    if (!visitor.ok()) {
      set_error(_KCCODELINE_, Error::DUPREC, "record duplication");
      return false;
    }
    return true;
  }
  /**
   * Set the value of a record.
   * @note Equal to the original DB::add method except that the parameters are std::string.
   */
  bool add(const std::string& key, const std::string& value) {
    _assert_(true);
    return add(key.c_str(), key.size(), value.c_str(), value.size());
  }
  /**
   * Replace the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, no new record is created and false is returned.
   * If the corresponding record exists, the value is modified.
   */
  bool replace(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(const char* vbuf, size_t vsiz) :
          vbuf_(vbuf), vsiz_(vsiz), ok_(false) {}
      bool ok() const {
        return ok_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        ok_ = true;
        *sp = vsiz_;
        return vbuf_;
      }
      const char* vbuf_;
      size_t vsiz_;
      bool ok_;
    };
    VisitorImpl visitor(vbuf, vsiz);
    if (!accept(kbuf, ksiz, &visitor, true)) return false;
    if (!visitor.ok()) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      return false;
    }
    return true;
  }
  /**
   * Replace the value of a record.
   * @note Equal to the original DB::replace method except that the parameters are std::string.
   */
  bool replace(const std::string& key, const std::string& value) {
    _assert_(true);
    return replace(key.c_str(), key.size(), value.c_str(), value.size());
  }
  /**
   * Append the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the given value is appended at the end of the existing value.
   */
  bool append(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(const char* vbuf, size_t vsiz) :
          vbuf_(vbuf), vsiz_(vsiz), nbuf_(NULL) {}
      ~VisitorImpl() {
        if (nbuf_) delete[] nbuf_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        size_t nsiz = vsiz + vsiz_;
        nbuf_ = new char[nsiz];
        std::memcpy(nbuf_, vbuf, vsiz);
        std::memcpy(nbuf_ + vsiz, vbuf_, vsiz_);
        *sp = nsiz;
        return nbuf_;
      }
      const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
        *sp = vsiz_;
        return vbuf_;
      }
      const char* vbuf_;
      size_t vsiz_;
      char* nbuf_;
    };
    VisitorImpl visitor(vbuf, vsiz);
    if (!accept(kbuf, ksiz, &visitor, true)) return false;
    return true;
  }
  /**
   * Set the value of a record.
   * @note Equal to the original DB::append method except that the parameters are std::string.
   */
  bool append(const std::string& key, const std::string& value) {
    _assert_(true);
    return append(key.c_str(), key.size(), value.c_str(), value.size());
  }
  /**
   * Add a number to the numeric value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param num the additional number.
   * @param orig the origin number if no record corresponds to the key.  If it is INT64MIN and
   * no record corresponds, this function fails.  If it is INT64MAX, the value is set as the
   * additional number regardless of the current value.
   * @return the result value, or kyotocabinet::INT64MIN on failure.
   * @note The value is serialized as an 8-byte binary integer in big-endian order, not a decimal
   * string.  If existing value is not 8-byte, this function fails.
   */
  int64_t increment(const char* kbuf, size_t ksiz, int64_t num, int64_t orig = 0) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(int64_t num, int64_t orig) : num_(num), orig_(orig), big_(0) {}
      int64_t num() {
        return num_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (vsiz != sizeof(num_)) {
          num_ = INT64MIN;
          return NOP;
        }
        int64_t onum;
        if (orig_ == INT64MAX) {
          onum = 0;
        } else {
          std::memcpy(&onum, vbuf, vsiz);
          onum = ntoh64(onum);
          if (num_ == 0) {
            num_ = onum;
            return NOP;
          }
        }
        num_ += onum;
        big_ = hton64(num_);
        *sp = sizeof(big_);
        return (const char*)&big_;
      }
      const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
        if (orig_ == INT64MIN) {
          num_ = INT64MIN;
          return NOP;
        }
        if (orig_ != INT64MAX) num_ += orig_;
        big_ = hton64(num_);
        *sp = sizeof(big_);
        return (const char*)&big_;
      }
      int64_t num_;
      int64_t orig_;
      uint64_t big_;
    };
    VisitorImpl visitor(num, orig);
    if (!accept(kbuf, ksiz, &visitor, num != 0 || orig != INT64MIN)) return INT64MIN;
    num = visitor.num();
    if (num == INT64MIN) {
      set_error(_KCCODELINE_, Error::LOGIC, "logical inconsistency");
      return num;
    }
    return num;
  }
  /**
   * Add a number to the numeric value of a record.
   * @note Equal to the original DB::increment method except that the parameter is std::string.
   */
  int64_t increment(const std::string& key, int64_t num, int64_t orig = 0) {
    _assert_(true);
    return increment(key.c_str(), key.size(), num, orig);
  }
  /**
   * Add a number to the numeric double value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param num the additional number.
   * @param orig the origin number if no record corresponds to the key.  If it is negative
   * infinity and no record corresponds, this function fails.  If it is positive infinity, the
   * value is set as the additional number regardless of the current value.
   * @return the result value, or Not-a-number on failure.
   * @note The value is serialized as an 16-byte binary fixed-point number in big-endian order,
   * not a decimal string.  If existing value is not 16-byte, this function fails.
   */
  double increment_double(const char* kbuf, size_t ksiz, double num, double orig = 0) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(double num, double orig) :
          DECUNIT(1000000000000000LL), num_(num), orig_(orig), buf_() {}
      double num() {
        return num_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (vsiz != sizeof(buf_)) {
          num_ = nan();
          return NOP;
        }
        int64_t linteg, lfract;
        if (chkinf(orig_) && orig_ >= 0) {
          linteg = 0;
          lfract = 0;
        } else {
          std::memcpy(&linteg, vbuf, sizeof(linteg));
          linteg = ntoh64(linteg);
          std::memcpy(&lfract, vbuf + sizeof(linteg), sizeof(lfract));
          lfract = ntoh64(lfract);
        }
        if (lfract == INT64MIN && linteg == INT64MIN) {
          num_ = nan();
          return NOP;
        } else if (linteg == INT64MAX) {
          num_ = HUGE_VAL;
          return NOP;
        } else if (linteg == INT64MIN) {
          num_ = -HUGE_VAL;
          return NOP;
        }
        if (num_ == 0.0 && !(chkinf(orig_) && orig_ >= 0)) {
          num_ = linteg + (double)lfract / DECUNIT;
          return NOP;
        }
        long double dinteg;
        long double dfract = std::modfl(num_, &dinteg);
        if (chknan(dinteg)) {
          linteg = INT64MIN;
          lfract = INT64MIN;
          num_ = nan();
        } else if (chkinf(dinteg)) {
          linteg = dinteg > 0 ? INT64MAX : INT64MIN;
          lfract = 0;
          num_ = dinteg;
        } else {
          linteg += (int64_t)dinteg;
          lfract += (int64_t)(dfract * DECUNIT);
          if (lfract >= DECUNIT) {
            linteg += 1;
            lfract -= DECUNIT;
          }
          num_ = linteg + (double)lfract / DECUNIT;
        }
        linteg = hton64(linteg);
        std::memcpy(buf_, &linteg, sizeof(linteg));
        lfract = hton64(lfract);
        std::memcpy(buf_ + sizeof(linteg), &lfract, sizeof(lfract));
        *sp = sizeof(buf_);
        return buf_;
      }
      const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
        if (chknan(orig_) || (chkinf(orig_) && orig_ < 0)) {
          num_ = nan();
          return NOP;
        }
        if (!chkinf(orig_)) num_ += orig_;
        long double dinteg;
        long double dfract = std::modfl(num_, &dinteg);
        int64_t linteg, lfract;
        if (chknan(dinteg)) {
          linteg = INT64MIN;
          lfract = INT64MIN;
        } else if (chkinf(dinteg)) {
          linteg = dinteg > 0 ? INT64MAX : INT64MIN;
          lfract = 0;
        } else {
          linteg = (int64_t)dinteg;
          lfract = (int64_t)(dfract * DECUNIT);
        }
        linteg = hton64(linteg);
        std::memcpy(buf_, &linteg, sizeof(linteg));
        lfract = hton64(lfract);
        std::memcpy(buf_ + sizeof(linteg), &lfract, sizeof(lfract));
        *sp = sizeof(buf_);
        return buf_;
      }
      const int64_t DECUNIT;
      double num_;
      double orig_;
      char buf_[sizeof(int64_t)*2];
    };
    VisitorImpl visitor(num, orig);
    if (!accept(kbuf, ksiz, &visitor, true)) return nan();
    num = visitor.num();
    if (chknan(num)) {
      set_error(_KCCODELINE_, Error::LOGIC, "logical inconsistency");
      return nan();
    }
    return num;
  }
  /**
   * Add a number to the numeric double value of a record.
   * @note Equal to the original DB::increment_double method except that the parameter is
   * std::string.
   */
  double increment_double(const std::string& key, double num, double orig) {
    _assert_(true);
    return increment_double(key.c_str(), key.size(), num, orig);
  }
  /**
   * Perform compare-and-swap.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param ovbuf the pointer to the old value region.  NULL means that no record corresponds.
   * @param ovsiz the size of the old value region.
   * @param nvbuf the pointer to the new value region.  NULL means that the record is removed.
   * @param nvsiz the size of new old value region.
   * @return true on success, or false on failure.
   */
  bool cas(const char* kbuf, size_t ksiz,
           const char* ovbuf, size_t ovsiz, const char* nvbuf, size_t nvsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(const char* ovbuf, size_t ovsiz, const char* nvbuf, size_t nvsiz) :
          ovbuf_(ovbuf), ovsiz_(ovsiz), nvbuf_(nvbuf), nvsiz_(nvsiz), ok_(false) {}
      bool ok() const {
        return ok_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (!ovbuf_ || vsiz != ovsiz_ || std::memcmp(vbuf, ovbuf_, vsiz)) return NOP;
        ok_ = true;
        if (!nvbuf_) return REMOVE;
        *sp = nvsiz_;
        return nvbuf_;
      }
      const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
        if (ovbuf_) return NOP;
        ok_ = true;
        if (!nvbuf_) return NOP;
        *sp = nvsiz_;
        return nvbuf_;
      }
      const char* ovbuf_;
      size_t ovsiz_;
      const char* nvbuf_;
      size_t nvsiz_;
      bool ok_;
    };
    VisitorImpl visitor(ovbuf, ovsiz, nvbuf, nvsiz);
    if (!accept(kbuf, ksiz, &visitor, true)) return false;
    if (!visitor.ok()) {
      set_error(_KCCODELINE_, Error::LOGIC, "status conflict");
      return false;
    }
    return true;
  }
  /**
   * Perform compare-and-swap.
   * @note Equal to the original DB::cas method except that the parameters are std::string.
   */
  bool cas(const std::string& key,
           const std::string& ovalue, const std::string& nvalue) {
    _assert_(true);
    return cas(key.c_str(), key.size(),
               ovalue.c_str(), ovalue.size(), nvalue.c_str(), nvalue.size());
  }
  /**
   * Remove a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, false is returned.
   */
  bool remove(const char* kbuf, size_t ksiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl() : ok_(false) {}
      bool ok() const {
        return ok_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        ok_ = true;
        return REMOVE;
      }
      bool ok_;
    };
    VisitorImpl visitor;
    if (!accept(kbuf, ksiz, &visitor, true)) return false;
    if (!visitor.ok()) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      return false;
    }
    return true;
  }
  /**
   * Remove a record.
   * @note Equal to the original DB::remove method except that the parameter is std::string.
   */
  bool remove(const std::string& key) {
    _assert_(true);
    return remove(key.c_str(), key.size());
  }
  /**
   * Retrieve the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @return the pointer to the value region of the corresponding record, or NULL on failure.
   * @note If no record corresponds to the key, NULL is returned.  Because an additional zero
   * code is appended at the end of the region of the return value, the return value can be
   * treated as a C-style string.  Because the region of the return value is allocated with the
   * the new[] operator, it should be released with the delete[] operator when it is no longer
   * in use.
   */
  char* get(const char* kbuf, size_t ksiz, size_t* sp) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && sp);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl() : vbuf_(NULL), vsiz_(0) {}
      char* pop(size_t* sp) {
        *sp = vsiz_;
        return vbuf_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        vbuf_ = new char[vsiz+1];
        std::memcpy(vbuf_, vbuf, vsiz);
        vbuf_[vsiz] = '\0';
        vsiz_ = vsiz;
        return NOP;
      }
      char* vbuf_;
      size_t vsiz_;
    };
    VisitorImpl visitor;
    if (!accept(kbuf, ksiz, &visitor, false)) {
      *sp = 0;
      return NULL;
    }
    size_t vsiz;
    char* vbuf = visitor.pop(&vsiz);
    if (!vbuf) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      *sp = 0;
      return NULL;
    }
    *sp = vsiz;
    return vbuf;
  }
  /**
   * Retrieve the value of a record.
   * @note Equal to the original DB::get method except that the first parameters is the key
   * string and the second parameter is a string to contain the result and the return value is
   * bool for success.
   */
  bool get(const std::string& key, std::string* value) {
    _assert_(value);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(std::string* value) : value_(value), ok_(false) {}
      bool ok() {
        return ok_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        value_->clear();
        value_->append(vbuf, vsiz);
        ok_ = true;
        return NOP;
      }
      std::string* value_;
      bool ok_;
    };
    VisitorImpl visitor(value);
    if (!accept(key.data(), key.size(), &visitor, false)) return false;
    if (!visitor.ok()) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      return false;
    }
    return true;
  }
  /**
   * Retrieve the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the buffer into which the value of the corresponding record is
   * written.
   * @param max the size of the buffer.
   * @return the size of the value, or -1 on failure.
   */
  int32_t get(const char* kbuf, size_t ksiz, char* vbuf, size_t max) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(char* vbuf, size_t max) : vbuf_(vbuf), max_(max), vsiz_(-1) {}
      int32_t vsiz() {
        return vsiz_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        vsiz_ = vsiz;
        size_t max = vsiz < max_ ? vsiz : max_;
        std::memcpy(vbuf_, vbuf, max);
        return NOP;
      }
      char* vbuf_;
      size_t max_;
      int32_t vsiz_;
    };
    VisitorImpl visitor(vbuf, max);
    if (!accept(kbuf, ksiz, &visitor, false)) return -1;
    int32_t vsiz = visitor.vsiz();
    if (vsiz < 0) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      return -1;
    }
    return vsiz;
  }
  /**
   * Check the existence of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @return the size of the value, or -1 on failure.
   */
  int32_t check(const char* kbuf, size_t ksiz) {
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl() : vsiz_(-1) {}
      int32_t vsiz() {
        return vsiz_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        vsiz_ = vsiz;
        return NOP;
      }
      size_t vsiz_;
    };
    VisitorImpl visitor;
    if (!accept(kbuf, ksiz, &visitor, false)) return -1;
    int32_t vsiz = visitor.vsiz();
    if (vsiz < 0) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      return -1;
    }
    return vsiz;
  }
  /**
   * Check the existence of a record.
   * @note Equal to the original DB::check method except that the parameter is std::string.
   */
  int32_t check(const std::string& key) {
    return check(key.data(), key.size());
  }
  /**
   * Retrieve the value of a record and remove it atomically.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @return the pointer to the value region of the corresponding record, or NULL on failure.
   * @note If no record corresponds to the key, NULL is returned.  Because an additional zero
   * code is appended at the end of the region of the return value, the return value can be
   * treated as a C-style string.  Because the region of the return value is allocated with the
   * the new[] operator, it should be released with the delete[] operator when it is no longer
   * in use.
   */
  char* seize(const char* kbuf, size_t ksiz, size_t* sp) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && sp);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl() : vbuf_(NULL), vsiz_(0) {}
      char* pop(size_t* sp) {
        *sp = vsiz_;
        return vbuf_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        vbuf_ = new char[vsiz+1];
        std::memcpy(vbuf_, vbuf, vsiz);
        vbuf_[vsiz] = '\0';
        vsiz_ = vsiz;
        return REMOVE;
      }
      char* vbuf_;
      size_t vsiz_;
    };
    VisitorImpl visitor;
    if (!accept(kbuf, ksiz, &visitor, true)) {
      *sp = 0;
      return NULL;
    }
    size_t vsiz;
    char* vbuf = visitor.pop(&vsiz);
    if (!vbuf) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      *sp = 0;
      return NULL;
    }
    *sp = vsiz;
    return vbuf;
  }
  /**
   * Retrieve the value of a record and remove it atomically.
   * @note Equal to the original DB::seize method except that the first parameters is the key
   * string and the second parameter is a string to contain the result and the return value is
   * bool for success.
   */
  bool seize(const std::string& key, std::string* value) {
    _assert_(value);
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(std::string* value) : value_(value), ok_(false) {}
      bool ok() {
        return ok_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        value_->clear();
        value_->append(vbuf, vsiz);
        ok_ = true;
        return REMOVE;
      }
      std::string* value_;
      bool ok_;
    };
    VisitorImpl visitor(value);
    if (!accept(key.data(), key.size(), &visitor, true)) return false;
    if (!visitor.ok()) {
      set_error(_KCCODELINE_, Error::NOREC, "no record");
      return false;
    }
    return true;
  }
  /**
   * Store records at once.
   * @param recs the records to store.
   * @param atomic true to perform all operations atomically, or false for non-atomic operations.
   * @return the number of stored records, or -1 on failure.
   */
  int64_t set_bulk(const std::map<std::string, std::string>& recs, bool atomic = true) {
    _assert_(true);
    if (atomic) {
      std::vector<std::string> keys;
      keys.reserve(recs.size());
      std::map<std::string, std::string>::const_iterator rit = recs.begin();
      std::map<std::string, std::string>::const_iterator ritend = recs.end();
      while (rit != ritend) {
        keys.push_back(rit->first);
        ++rit;
      }
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl(const std::map<std::string, std::string>& recs) : recs_(recs) {}
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          std::map<std::string, std::string>::const_iterator rit =
              recs_.find(std::string(kbuf, ksiz));
          if (rit == recs_.end()) return NOP;
          *sp = rit->second.size();
          return rit->second.data();
        }
        const char* visit_empty(const char* kbuf, size_t ksiz, size_t* sp) {
          std::map<std::string, std::string>::const_iterator rit =
              recs_.find(std::string(kbuf, ksiz));
          if (rit == recs_.end()) return NOP;
          *sp = rit->second.size();
          return rit->second.data();
        }
        const std::map<std::string, std::string>& recs_;
      };
      VisitorImpl visitor(recs);
      if (!accept_bulk(keys, &visitor, true)) return -1;
      return keys.size();
    }
    std::map<std::string, std::string>::const_iterator rit = recs.begin();
    std::map<std::string, std::string>::const_iterator ritend = recs.end();
    while (rit != ritend) {
      if (!set(rit->first.data(), rit->first.size(), rit->second.data(), rit->second.size()))
        return -1;
      ++rit;
    }
    return recs.size();
  }
  /**
   * Remove records at once.
   * @param keys the keys of the records to remove.
   * @param atomic true to perform all operations atomically, or false for non-atomic operations.
   * @return the number of removed records, or -1 on failure.
   */
  int64_t remove_bulk(const std::vector<std::string>& keys, bool atomic = true) {
    _assert_(true);
    if (atomic) {
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl() : cnt_(0) {}
        int64_t cnt() const {
          return cnt_;
        }
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          cnt_++;
          return REMOVE;
        }
        int64_t cnt_;
      };
      VisitorImpl visitor;
      if (!accept_bulk(keys, &visitor, true)) return -1;
      return visitor.cnt();
    }
    int64_t cnt = 0;
    std::vector<std::string>::const_iterator kit = keys.begin();
    std::vector<std::string>::const_iterator kitend = keys.end();
    while (kit != kitend) {
      if (remove(kit->data(), kit->size())) {
        cnt++;
      } else if (error() != Error::NOREC) {
        return -1;
      }
      ++kit;
    }
    return cnt;
  }
  /**
   * Retrieve records at once.
   * @param keys the keys of the records to retrieve.
   * @param recs a string map to contain the retrieved records.
   * @param atomic true to perform all operations atomically, or false for non-atomic operations.
   * @return the number of retrieved records, or -1 on failure.
   */
  int64_t get_bulk(const std::vector<std::string>& keys,
                   std::map<std::string, std::string>* recs, bool atomic = true) {
    _assert_(recs);
    if (atomic) {
      class VisitorImpl : public Visitor {
       public:
        explicit VisitorImpl(std::map<std::string, std::string>* recs) : recs_(recs) {}
       private:
        const char* visit_full(const char* kbuf, size_t ksiz,
                               const char* vbuf, size_t vsiz, size_t* sp) {
          (*recs_)[std::string(kbuf, ksiz)] = std::string(vbuf, vsiz);
          return NOP;
        }
        std::map<std::string, std::string>* recs_;
      };
      VisitorImpl visitor(recs);
      if (!accept_bulk(keys, &visitor, false)) return -1;
      return recs->size();
    }
    std::vector<std::string>::const_iterator kit = keys.begin();
    std::vector<std::string>::const_iterator kitend = keys.end();
    while (kit != kitend) {
      size_t vsiz;
      const char* vbuf = get(kit->data(), kit->size(), &vsiz);
      if (vbuf) {
        (*recs)[*kit] = std::string(vbuf, vsiz);
        delete[] vbuf;
      } else if (error() != Error::NOREC) {
        return -1;
      }
      ++kit;
    }
    return recs->size();
  }
  /**
   * Dump records into a data stream.
   * @param dest the destination stream.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   */
  bool dump_snapshot(std::ostream* dest, ProgressChecker* checker = NULL) {
    _assert_(dest);
    if (dest->fail()) {
      set_error(_KCCODELINE_, Error::INVALID, "invalid stream");
      return false;
    }
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(std::ostream* dest) : dest_(dest), stack_() {}
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        char* wp = stack_;
        *(wp++) = 0x00;
        wp += writevarnum(wp, ksiz);
        wp += writevarnum(wp, vsiz);
        dest_->write(stack_, wp - stack_);
        dest_->write(kbuf, ksiz);
        dest_->write(vbuf, vsiz);
        return NOP;
      }
      std::ostream* dest_;
      char stack_[NUMBUFSIZ*2];
    };
    VisitorImpl visitor(dest);
    bool err = false;
    dest->write(KCDBSSMAGICDATA, sizeof(KCDBSSMAGICDATA));
    if (iterate(&visitor, false, checker)) {
      unsigned char c = 0xff;
      dest->write((char*)&c, 1);
      if (dest->fail()) {
        set_error(_KCCODELINE_, Error::SYSTEM, "stream output error");
        err = true;
      }
    } else {
      err = true;
    }
    return !err;
  }
  /**
   * Dump records into a file.
   * @param dest the path of the destination file.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   */
  bool dump_snapshot(const std::string& dest, ProgressChecker* checker = NULL) {
    _assert_(true);
    std::ofstream ofs;
    ofs.open(dest.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    if (!ofs) {
      set_error(_KCCODELINE_, Error::NOREPOS, "open failed");
      return false;
    }
    bool err = false;
    if (!dump_snapshot(&ofs, checker)) err = true;
    ofs.close();
    if (!ofs) {
      set_error(_KCCODELINE_, Error::SYSTEM, "close failed");
      err = true;
    }
    return !err;
  }
  /**
   * Load records from a data stream.
   * @param src the source stream.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   */
  bool load_snapshot(std::istream* src, ProgressChecker* checker = NULL) {
    _assert_(src);
    if (src->fail()) {
      set_error(_KCCODELINE_, Error::INVALID, "invalid stream");
      return false;
    }
    char buf[IOBUFSIZ];
    src->read(buf, sizeof(KCDBSSMAGICDATA));
    if (src->fail()) {
      set_error(_KCCODELINE_, Error::SYSTEM, "stream input error");
      return false;
    }
    if (std::memcmp(buf, KCDBSSMAGICDATA, sizeof(KCDBSSMAGICDATA))) {
      set_error(_KCCODELINE_, Error::INVALID, "invalid magic data of input stream");
      return false;
    }
    bool err = false;
    if (checker && !checker->check("load_snapshot", "beginning", 0, -1)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      err = true;
    }
    int64_t curcnt = 0;
    while (!err) {
      int32_t c = src->get();
      if (src->fail()) {
        set_error(_KCCODELINE_, Error::SYSTEM, "stream input error");
        err = true;
        break;
      }
      if (c == 0xff) break;
      if (c == 0x00) {
        size_t ksiz = 0;
        do {
          c = src->get();
          ksiz = (ksiz << 7) + (c & 0x7f);
        } while (c >= 0x80);
        size_t vsiz = 0;
        do {
          c = src->get();
          vsiz = (vsiz << 7) + (c & 0x7f);
        } while (c >= 0x80);
        size_t rsiz = ksiz + vsiz;
        char* rbuf = rsiz > sizeof(buf) ? new char[rsiz] : buf;
        src->read(rbuf, ksiz + vsiz);
        if (src->fail()) {
          set_error(_KCCODELINE_, Error::SYSTEM, "stream input error");
          err = true;
          if (rbuf != buf) delete[] rbuf;
          break;
        }
        if (!set(rbuf, ksiz, rbuf + ksiz, vsiz)) {
          err = true;
          if (rbuf != buf) delete[] rbuf;
          break;
        }
        if (rbuf != buf) delete[] rbuf;
      } else {
        set_error(_KCCODELINE_, Error::INVALID, "invalid magic data of input stream");
        err = true;
        break;
      }
      curcnt++;
      if (checker && !checker->check("load_snapshot", "processing", curcnt, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        err = true;
        break;
      }
    }
    if (checker && !checker->check("load_snapshot", "ending", -1, -1)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      err = true;
    }
    return !err;
  }
  /**
   * Load records from a file.
   * @param src the path of the source file.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   */
  bool load_snapshot(const std::string& src, ProgressChecker* checker = NULL) {
    _assert_(true);
    std::ifstream ifs;
    ifs.open(src.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!ifs) {
      set_error(_KCCODELINE_, Error::NOREPOS, "open failed");
      return false;
    }
    bool err = false;
    if (!load_snapshot(&ifs, checker)) err = true;
    ifs.close();
    if (ifs.bad()) {
      set_error(_KCCODELINE_, Error::SYSTEM, "close failed");
      return false;
    }
    return !err;
  }
  /**
   * Create a cursor object.
   * @return the return value is the created cursor object.
   * @note Because the object of the return value is allocated by the constructor, it should be
   * released with the delete operator when it is no longer in use.
   */
  virtual Cursor* cursor() = 0;
  /**
   * Write a log message.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param message the supplement message.
   */
  virtual void log(const char* file, int32_t line, const char* func, Logger::Kind kind,
                   const char* message) = 0;
  /**
   * Set the internal logger.
   * @param logger the logger object.
   * @param kinds kinds of logged messages by bitwise-or: Logger::DEBUG for debugging,
   * Logger::INFO for normal information, Logger::WARN for warning, and Logger::ERROR for fatal
   * error.
   * @return true on success, or false on failure.
   */
  virtual bool tune_logger(Logger* logger, uint32_t kinds = Logger::WARN | Logger::ERROR) = 0;
  /**
   * Set the internal meta operation trigger.
   * @param trigger the trigger object.
   * @return true on success, or false on failure.
   */
  virtual bool tune_meta_trigger(MetaTrigger* trigger) = 0;
  /**
   * Get the class name of a database type.
   * @param type the database type.
   * @return the string of the type name.
   */
  static const char* typecname(uint32_t type) {
    _assert_(true);
    switch (type) {
      case TYPEVOID: return "void";
      case TYPEPHASH: return "ProtoHashDB";
      case TYPEPTREE: return "ProtoTreeDB";
      case TYPESTASH: return "StashDB";
      case TYPECACHE: return "CacheDB";
      case TYPEGRASS: return "GrassDB";
      case TYPEHASH: return "HashDB";
      case TYPETREE: return "TreeDB";
      case TYPEDIR: return "DirDB";
      case TYPEFOREST: return "ForestDB";
      case TYPEMISC: return "misc";
    }
    return "unknown";
  }
  /**
   * Get the description string of a database type.
   * @param type the database type.
   * @return the string of the type name.
   */
  static const char* typestring(uint32_t type) {
    _assert_(true);
    switch (type) {
      case TYPEVOID: return "void";
      case TYPEPHASH: return "prototype hash database";
      case TYPEPTREE: return "prototype tree database";
      case TYPESTASH: return "stash database";
      case TYPECACHE: return "cache hash database";
      case TYPEGRASS: return "cache tree database";
      case TYPEHASH: return "file hash database";
      case TYPETREE: return "file tree database";
      case TYPEDIR: return "directory hash database";
      case TYPEFOREST: return "directory tree database";
      case TYPEMISC: return "miscellaneous database";
    }
    return "unknown";
  }
};


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
