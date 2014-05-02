/*************************************************************************
 *
 * TIGHTDB CONFIDENTIAL
 * __________________
 *
 *  [2011] - [2012] TightDB Inc
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of TightDB Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to TightDB Incorporated
 * and its suppliers and may be covered by U.S. and Foreign Patents,
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from TightDB Incorporated.
 *
 **************************************************************************/
#ifndef TIGHTDB_REPLICATION_HPP
#define TIGHTDB_REPLICATION_HPP

#include <algorithm>
#include <limits>
#include <exception>
#include <string>

#include <tightdb/util/assert.hpp>
#include <tightdb/util/tuple.hpp>
#include <tightdb/util/safe_int_ops.hpp>
#include <tightdb/util/unique_ptr.hpp>
#include <tightdb/util/buffer.hpp>
#include <tightdb/util/string_buffer.hpp>
#include <tightdb/util/file.hpp>
#include <tightdb/group.hpp>
#include <tightdb/descriptor.hpp>
#include <tightdb/group.hpp>
#include <tightdb/group_shared.hpp>

namespace tightdb {



// FIXME: Be careful about the possibility of one modification functions being called by another where both do transaction logging.

// FIXME: How to handle unordered tables. Will it introduce nondeterminism in actual order such that indices cannot be considered equal across replication instances?

// FIXME: The current table/subtable selection scheme assumes that a TableRef of a subtable is not accessed after any modification of one of its ancestor tables.

// FIXME: Checking on same Table* requires that ~Table checks and nullifies on match. Another option would be to store m_selected_table as a TableRef. Yet another option would be to assign unique identifiers to each Table instance vial Allocator. Yet another option would be to explicitely invalidate subtables recursively when parent is modified.



/// Replication is enabled by passing an instance of an implementation
/// of this class to the SharedGroup constructor.
class Replication {
public:
    // Be sure to keep this type aligned with what is actually used in
    // SharedGroup.
    typedef uint_fast64_t version_type;

    std::string get_database_path();

    class Interrupted; // Exception

    /// Acquire permision to start a new 'write' transaction. This
    /// function must be called by a client before it requests a
    /// 'write' transaction. This ensures that the local shared
    /// database is up-to-date. During the transaction, all
    /// modifications must be posted to this Replication instance as
    /// calls to set_value() and friends. After the completion of the
    /// transaction, the client must call either
    /// commit_write_transact() or rollback_write_transact().
    ///
    /// \throw Interrupted If this call was interrupted by an
    /// asynchronous call to interrupt().
    void begin_write_transact(SharedGroup&);

    /// Commit the accumulated transaction log. The transaction log
    /// may not be committed if any of the functions that submit data
    /// to it, have failed or been interrupted. This operation will
    /// block until the local coordinator reports that the transaction
    /// log has been dealt with in a manner that makes the transaction
    /// persistent. This operation may be interrupted by an
    /// asynchronous call to interrupt().
    ///
    /// \throw Interrupted If this call was interrupted by an
    /// asynchronous call to interrupt().
    ///
    /// FIXME: In general the transaction will be considered complete
    /// even if this operation is interrupted. Is that ok?
    version_type commit_write_transact(SharedGroup&, version_type orig_version);

    /// Called by a client to discard the accumulated transaction
    /// log. This function must be called if a write transaction was
    /// successfully initiated, but one of the functions that submit
    /// data to the transaction log has failed or has been
    /// interrupted. It must also be called after a failed or
    /// interrupted call to commit_write_transact().
    void rollback_write_transact(SharedGroup&) TIGHTDB_NOEXCEPT;

    /// Interrupt any blocking call to a function in this class. This
    /// function may be called asyncronously from any thread, but it
    /// may not be called from a system signal handler.
    ///
    /// Some of the public function members of this class may block,
    /// but only when it it is explicitely stated in the documention
    /// for those functions.
    ///
    /// FIXME: Currently we do not state blocking behaviour for all
    /// the functions that can block.
    ///
    /// After any function has returned with an interruption
    /// indication, the only functions that may safely be called are
    /// rollback_write_transact() and the destructor. If a client,
    /// after having received an interruption indication, calls
    /// rollback_write_transact() and then clear_interrupt(), it may
    /// resume normal operation through this Replication instance.
    void interrupt() TIGHTDB_NOEXCEPT;

    /// May be called by a client to reset this replication instance
    /// after an interrupted transaction. It is not an error to call
    /// this function in a situation where no interruption has
    /// occured.
    void clear_interrupt() TIGHTDB_NOEXCEPT;

    void new_group_level_table(StringData name);
    void insert_column(const Descriptor&, std::size_t column_ndx, DataType type, StringData name);
    void erase_column(const Descriptor&, std::size_t column_ndx);
    void rename_column(const Descriptor&, std::size_t column_ndx, StringData name);

    void set_int(const Table*, std::size_t column_ndx, std::size_t ndx, int_fast64_t value);
    void set_bool(const Table*, std::size_t column_ndx, std::size_t ndx, bool value);
    void set_float(const Table*, std::size_t column_ndx, std::size_t ndx, float value);
    void set_double(const Table*, std::size_t column_ndx, std::size_t ndx, double value);
    void set_string(const Table*, std::size_t column_ndx, std::size_t ndx, StringData value);
    void set_binary(const Table*, std::size_t column_ndx, std::size_t ndx, BinaryData value);
    void set_date_time(const Table*, std::size_t column_ndx, std::size_t ndx, DateTime value);
    void set_table(const Table*, std::size_t column_ndx, std::size_t ndx);
    void set_mixed(const Table*, std::size_t column_ndx, std::size_t ndx, const Mixed& value);

    void insert_int(const Table*, std::size_t column_ndx, std::size_t ndx, int_fast64_t value);
    void insert_bool(const Table*, std::size_t column_ndx, std::size_t ndx, bool value);
    void insert_float(const Table*, std::size_t column_ndx, std::size_t ndx, float value);
    void insert_double(const Table*, std::size_t column_ndx, std::size_t ndx, double value);
    void insert_string(const Table*, std::size_t column_ndx, std::size_t ndx, StringData value);
    void insert_binary(const Table*, std::size_t column_ndx, std::size_t ndx, BinaryData value);
    void insert_date_time(const Table*, std::size_t column_ndx, std::size_t ndx, DateTime value);
    void insert_table(const Table*, std::size_t column_ndx, std::size_t ndx);
    void insert_mixed(const Table*, std::size_t column_ndx, std::size_t ndx, const Mixed& value);

    void row_insert_complete(const Table*);
    void insert_empty_rows(const Table*, std::size_t row_ndx, std::size_t num_rows);
    void erase_row(const Table*, std::size_t row_ndx);
    void add_int_to_column(const Table*, std::size_t column_ndx, int_fast64_t value);
    void add_index_to_column(const Table*, std::size_t column_ndx);
    void clear_table(const Table*);
    void optimize_table(const Table*);

    void on_table_destroyed(const Table*) TIGHTDB_NOEXCEPT;
    void on_spec_destroyed(const Spec*) TIGHTDB_NOEXCEPT;


    class TransactLogParser;

    class InputStream;

    class BadTransactLog; // Exception

    /// Called by the local coordinator to apply a transaction log
    /// received from another local coordinator.
    ///
    /// \param apply_log If specified, and the library was compiled in
    /// debug mode, then a line describing each individual operation
    /// is writted to the specified stream.
    ///
    /// \throw BadTransactLog If the transaction log could not be
    /// successfully parsed, or ended prematurely.
    static void apply_transact_log(InputStream& transact_log, Group& target,
                                   std::ostream* apply_log = 0);

    virtual ~Replication() TIGHTDB_NOEXCEPT {}

protected:
    // These two delimit a contiguous region of free space in a
    // transaction log buffer following the last written data. It may
    // be empty.
    char* m_transact_log_free_begin;
    char* m_transact_log_free_end;

    Replication();

    virtual std::string do_get_database_path() = 0;

    /// As part of the initiation of a write transaction, this method
    /// is supposed to update `m_transact_log_free_begin` and
    /// `m_transact_log_free_end` such that they refer to a (possibly
    /// empty) chunk of free space.
    virtual void do_begin_write_transact(SharedGroup&) = 0;

    /// The caller guarantees that `m_transact_log_free_begin` marks
    /// the end of payload data in the transaction log.
    virtual version_type do_commit_write_transact(SharedGroup&, version_type orig_version) = 0;

    virtual void do_rollback_write_transact(SharedGroup&) TIGHTDB_NOEXCEPT = 0;

    virtual void do_interrupt() TIGHTDB_NOEXCEPT = 0;

    virtual void do_clear_interrupt() TIGHTDB_NOEXCEPT = 0;

    /// Ensure contiguous free space in the transaction log
    /// buffer. This method must update `m_transact_log_free_begin`
    /// and `m_transact_log_free_end` such that they refer to a chunk
    /// of free space whose size is at least \a n.
    ///
    /// \param n The required amount of contiguous free space. Must be
    /// small (probably not greater than 1024)
    virtual void do_transact_log_reserve(std::size_t n) = 0;

    /// Copy the specified data into the transaction log buffer. This
    /// function should be called only when the specified data does
    /// not fit inside the chunk of free space currently referred to
    /// by `m_transact_log_free_begin` and `m_transact_log_free_end`.
    ///
    /// This method must update `m_transact_log_free_begin` and
    /// `m_transact_log_free_end` such that, upon return, they still
    /// refer to a (possibly empty) chunk of free space.
    virtual void do_transact_log_append(const char* data, std::size_t size) = 0;

    /// Must be called only from do_begin_write_transact(),
    /// do_commit_write_transact(), or do_rollback_write_transact().
    static Group& get_group(SharedGroup&) TIGHTDB_NOEXCEPT;

    // Part of a temporary ugly hack to avoid generating new
    // transaction logs during application of ones that have olready
    // been created elsewhere. See
    // ReplicationImpl::do_begin_write_transact() in
    // tigthdb/replication/simplified/provider.cpp for more on this.
    static void set_replication(Group&, Replication*) TIGHTDB_NOEXCEPT;

    /// Must be called only from do_begin_write_transact(),
    /// do_commit_write_transact(), or do_rollback_write_transact().
    static version_type get_current_version(SharedGroup&);

private:
    class TransactLogApplier;

    /// Transaction log instruction encoding
    enum Instruction {
        instr_NewGroupLevelTable =  1,
        instr_SelectTable        =  2,
        instr_SetInt             =  3,
        instr_SetBool            =  4,
        instr_SetFloat           =  5,
        instr_SetDouble          =  6,
        instr_SetString          =  7,
        instr_SetBinary          =  8,
        instr_SetDateTime        =  9,
        instr_SetTable           = 10,
        instr_SetMixed           = 11,
        instr_InsertInt          = 12,
        instr_InsertBool         = 13,
        instr_InsertFloat        = 14,
        instr_InsertDouble       = 15,
        instr_InsertString       = 16,
        instr_InsertBinary       = 17,
        instr_InsertDateTime     = 18,
        instr_InsertTable        = 19,
        instr_InsertMixed        = 20,
        instr_RowInsertComplete  = 21,
        instr_InsertEmptyRows    = 22,
        instr_EraseRow           = 23, // Remove a row
        instr_AddIntToColumn     = 24, // Add an integer value to all cells in a column
        instr_ClearTable         = 25, // Remove all rows in selected table
        instr_OptimizeTable      = 26,
        instr_SelectDescriptor   = 27, // Select descriptor from currently selected root table
        instr_InsertColumn       = 28, // Insert new column into to selected descriptor
        instr_EraseColumn        = 29, // Remove column from selected descriptor
        instr_RenameColumn       = 30, // Rename column in selected descriptor
        instr_AddIndexToColumn   = 31  // Add a lookup index to a column
    };

    util::Buffer<std::size_t> m_subtab_path_buf;

    const Table* m_selected_table;
    const Spec*  m_selected_spec;

    /// \param n Must be small (probably not greater than 1024)
    void transact_log_reserve(char** buf, int n);

    /// \param ptr Must be in the rangle [m_transact_log_free_begin, m_transact_log_free_end]
    void transact_log_advance(char* ptr) TIGHTDB_NOEXCEPT;

    void transact_log_append(const char* data, std::size_t size);

    void check_table(const Table*);
    void select_table(const Table*); // Deselects a selected spec

    void check_desc(const Descriptor& desc);
    void select_desc(const Descriptor& desc);

    void string_cmd(Instruction, std::size_t column_ndx, std::size_t ndx,
                    const char* data, std::size_t size);

    void mixed_cmd(Instruction, std::size_t column_ndx, std::size_t ndx, const Mixed& value);

    template<class L> void simple_cmd(Instruction, const util::Tuple<L>& numbers);

    template<class> struct EncodeNumber;

    template<class T> static char* encode_int(char* ptr, T value);

    static char* encode_float(char* ptr, float value);
    static char* encode_double(char* ptr, double value);

    // Make sure this is in agreement with the actual integer encoding
    // scheme (see encode_int()).
    static const int max_enc_bytes_per_int = 10;
    static const int max_enc_bytes_per_double = sizeof (double);
    static const int max_enc_bytes_per_num = max_enc_bytes_per_int <
        max_enc_bytes_per_double ? max_enc_bytes_per_double : max_enc_bytes_per_int;
};


class Replication::InputStream {
public:
    /// \return The number of extracted bytes. This will always be
    /// less than or equal to \a size. A value of zero indicates
    /// end-of-input unless \a size was zero.
    virtual std::size_t read(char* buffer, std::size_t size) = 0;

    virtual ~InputStream() {}
};


class Replication::Interrupted: public std::exception {
public:
    const char* what() const TIGHTDB_NOEXCEPT_OR_NOTHROW TIGHTDB_OVERRIDE
    {
        return "Interrupted";
    }
};

class Replication::BadTransactLog: public std::exception {
public:
    const char* what() const TIGHTDB_NOEXCEPT_OR_NOTHROW TIGHTDB_OVERRIDE
    {
        return "Bad transaction log";
    }
};


class Replication::TransactLogParser {
public:
    TransactLogParser(InputStream& transact_log);

    ~TransactLogParser() TIGHTDB_NOEXCEPT;

    template<class InstructionHandler> void parse(InstructionHandler&);

private:
    InputStream& m_input;
    static const std::size_t m_input_buffer_size = 4096; // FIXME: Use smaller number when compiling in debug mode
    util::UniquePtr<char[]> m_input_buffer;
    const char* m_input_begin;
    const char* m_input_end;
    util::StringBuffer m_string_buffer;
    static const int m_max_levels = 1024;
    util::Buffer<std::size_t> m_path;

    template<class T> T read_int();

    void read_bytes(char* data, std::size_t size);

    float read_float();
    double read_double();

    void read_string(util::StringBuffer&);
    void read_mixed(Mixed*);

    // Returns false if no more input was available
    bool fill_input_buffer();

    // Returns false if no input was available
    bool read_char(char&);

    bool is_valid_column_type(int type);
};


class TrivialReplication: public Replication {
public:
    ~TrivialReplication() TIGHTDB_NOEXCEPT {}

protected:
    TrivialReplication(const std::string& database_file);

    virtual void handle_transact_log(const char* data, std::size_t size) = 0;

    static void apply_transact_log(const char* data, std::size_t size, SharedGroup& target,
                                   std::ostream* apply_log = 0);

private:
    const std::string m_database_file;
    util::Buffer<char> m_transact_log_buffer;

    std::string do_get_database_path() TIGHTDB_OVERRIDE;
    void do_begin_write_transact(SharedGroup&) TIGHTDB_OVERRIDE;
    version_type do_commit_write_transact(SharedGroup&, version_type orig_version) TIGHTDB_OVERRIDE;
    void do_rollback_write_transact(SharedGroup&) TIGHTDB_NOEXCEPT TIGHTDB_OVERRIDE;
    void do_interrupt() TIGHTDB_NOEXCEPT TIGHTDB_OVERRIDE;
    void do_clear_interrupt() TIGHTDB_NOEXCEPT TIGHTDB_OVERRIDE;
    void do_transact_log_reserve(std::size_t n) TIGHTDB_OVERRIDE;
    void do_transact_log_append(const char* data, std::size_t size) TIGHTDB_OVERRIDE;

    void transact_log_reserve(std::size_t);
};





// Implementation:

inline std::string Replication::get_database_path()
{
    return do_get_database_path();
}


inline void Replication::begin_write_transact(SharedGroup& sg)
{
    do_begin_write_transact(sg);
    m_selected_table = 0;
    m_selected_spec  = 0;
}

inline Replication::version_type
Replication::commit_write_transact(SharedGroup& sg, version_type orig_version)
{
    return do_commit_write_transact(sg, orig_version);
}

inline void Replication::rollback_write_transact(SharedGroup& sg) TIGHTDB_NOEXCEPT
{
    do_rollback_write_transact(sg);
}

inline void Replication::interrupt() TIGHTDB_NOEXCEPT
{
    do_interrupt();
}

inline void Replication::clear_interrupt() TIGHTDB_NOEXCEPT
{
    do_clear_interrupt();
}


inline void Replication::transact_log_reserve(char** buf, int n)
{
    if (std::size_t(m_transact_log_free_end - m_transact_log_free_begin) < std::size_t(n))
        do_transact_log_reserve(n); // Throws
    *buf = m_transact_log_free_begin;
}


inline void Replication::transact_log_advance(char* ptr) TIGHTDB_NOEXCEPT
{
    TIGHTDB_ASSERT(m_transact_log_free_begin <= ptr);
    TIGHTDB_ASSERT(ptr <= m_transact_log_free_end);
    m_transact_log_free_begin = ptr;
}


inline void Replication::transact_log_append(const char* data, std::size_t size)
{
    if (TIGHTDB_UNLIKELY(std::size_t(m_transact_log_free_end -
                                     m_transact_log_free_begin) < size)) {
        do_transact_log_append(data, size); // Throws
        return;
    }
    m_transact_log_free_begin = std::copy(data, data+size, m_transact_log_free_begin);
}


// The integer encoding is platform independent. Also, it does not
// depend on the type of the specified integer. Integers of any type
// can be encoded as long as the specified buffer is large enough (see
// below). The decoding does not have to use the same type. Decoding
// will fail if, and only if the encoded value falls outside the range
// of the requested destination type.
//
// The encoding uses one or more bytes. It never uses more than 8 bits
// per byte. The last byte in the sequence is the first one that has
// its 8th bit set to zero.
//
// Consider a particular non-negative value V. Let W be the number of
// bits needed to encode V using the trivial binary encoding of
// integers. The total number of bytes produced is then
// ceil((W+1)/7). The first byte holds the 7 least significant bits of
// V. The last byte holds at most 6 bits of V including the most
// significant one. The value of the first bit of the last byte is
// always 2**((N-1)*7) where N is the total number of bytes.
//
// A negative value W is encoded by setting the sign bit to one and
// then encoding the positive result of -(W+1) as described above. The
// advantage of this representation is that it converts small negative
// values to small positive values which require a small number of
// bytes. This would not have been true for 2's complements
// representation, for example. The sign bit is always stored as the
// 7th bit of the last byte.
//
//               value bits    value + sign    max bytes
//     --------------------------------------------------
//     int8_t         7              8              2
//     uint8_t        8              9              2
//     int16_t       15             16              3
//     uint16_t      16             17              3
//     int32_t       31             32              5
//     uint32_t      32             33              5
//     int64_t       63             64             10
//     uint64_t      64             65             10
//
template<class T> inline char* Replication::encode_int(char* ptr, T value)
{
    TIGHTDB_STATIC_ASSERT(std::numeric_limits<T>::is_integer, "Integer required");
    bool negative = false;
    if (util::is_negative(value)) {
        negative = true;
        // The following conversion is guaranteed by C++11 to never
        // overflow (contrast this with "-value" which indeed could
        // overflow). See C99+TC3 section 6.2.6.2 paragraph 2.
        value = -(value + 1);
    }
    // At this point 'value' is always a positive number. Also, small
    // negative numbers have been converted to small positive numbers.
    TIGHTDB_ASSERT(!util::is_negative(value));
    // One sign bit plus number of value bits
    const int num_bits = 1 + std::numeric_limits<T>::digits;
    // Only the first 7 bits are available per byte. Had it not been
    // for the fact that maximum guaranteed bit width of a char is 8,
    // this value could have been increased to 15 (one less than the
    // number of value bits in 'unsigned').
    const int bits_per_byte = 7;
    const int max_bytes = (num_bits + (bits_per_byte-1)) / bits_per_byte;
    TIGHTDB_STATIC_ASSERT(max_bytes <= max_enc_bytes_per_int, "Bad max_enc_bytes_per_int");
    // An explicit constant maximum number of iterations is specified
    // in the hope that it will help the optimizer (to do loop
    // unrolling, for example).
    for (int i=0; i<max_bytes; ++i) {
        if (value >> (bits_per_byte-1) == 0)
            break;
        *reinterpret_cast<unsigned char*>(ptr) =
            (1U<<bits_per_byte) | unsigned(value & ((1U<<bits_per_byte)-1));
        ++ptr;
        value >>= bits_per_byte;
    }
    *reinterpret_cast<unsigned char*>(ptr) =
        negative ? (1U<<(bits_per_byte-1)) | unsigned(value) : value;
    return ++ptr;
}


inline char* Replication::encode_float(char* ptr, float value)
{
    TIGHTDB_STATIC_ASSERT(std::numeric_limits<float>::is_iec559 &&
                          sizeof (float) * std::numeric_limits<unsigned char>::digits == 32,
                          "Unsupported 'float' representation");
    const char* val_ptr = reinterpret_cast<char*>(&value);
    return std::copy(val_ptr, val_ptr + sizeof value, ptr);
}


inline char* Replication::encode_double(char* ptr, double value)
{
    TIGHTDB_STATIC_ASSERT(std::numeric_limits<double>::is_iec559 &&
                          sizeof (double) * std::numeric_limits<unsigned char>::digits == 64,
                          "Unsupported 'double' representation");
    const char* val_ptr = reinterpret_cast<char*>(&value);
    return std::copy(val_ptr, val_ptr + sizeof value, ptr);
}


template<class T> struct Replication::EncodeNumber {
    void operator()(T value, char** ptr)
    {
        *ptr = encode_int(*ptr, value);
    }
};
template<> struct Replication::EncodeNumber<float> {
    void operator()(float  value, char** ptr)
    {
        *ptr = encode_float(*ptr, value);
    }
};
template<> struct Replication::EncodeNumber<double> {
    void operator()(double value, char** ptr)
    {
        *ptr = encode_double(*ptr, value);
    }
};


template<class L>
inline void Replication::simple_cmd(Instruction instr, const util::Tuple<L>& numbers)
{
    char* buf;
    transact_log_reserve(&buf, 1 + util::TypeCount<L>::value*max_enc_bytes_per_num); // Throws
    *buf++ = char(instr);
    util::for_each<EncodeNumber>(numbers, &buf);
    transact_log_advance(buf);
}


inline void Replication::check_table(const Table* t)
{
    if (t != m_selected_table)
        select_table(t); // Throws
}


inline void Replication::check_desc(const Descriptor& desc)
{
    if (desc.m_spec != m_selected_spec)
        select_desc(desc); // Throws
}


inline void Replication::string_cmd(Instruction instr, std::size_t column_ndx,
                                    std::size_t ndx, const char* data, std::size_t size)
{
    simple_cmd(instr, util::tuple(column_ndx, ndx, size)); // Throws
    transact_log_append(data, size); // Throws
}


inline void Replication::mixed_cmd(Instruction instr, std::size_t column_ndx,
                                   std::size_t ndx, const Mixed& value)
{
    DataType type = value.get_type();
    char* buf;
    transact_log_reserve(&buf, 1 + 4*max_enc_bytes_per_num); // Throws
    *buf++ = char(instr);
    buf = encode_int(buf, column_ndx);
    buf = encode_int(buf, ndx);
    buf = encode_int(buf, int(type));
    switch (type) {
        case type_Int:
            buf = encode_int(buf, value.get_int());
            transact_log_advance(buf);
            return;
        case type_Bool:
            buf = encode_int(buf, int(value.get_bool()));
            transact_log_advance(buf);
            return;
        case type_Float:
            buf = encode_float(buf, value.get_float());
            transact_log_advance(buf);
            return;
        case type_Double:
            buf = encode_double(buf, value.get_double());
            transact_log_advance(buf);
            return;
        case type_DateTime:
            buf = encode_int(buf, value.get_datetime().get_datetime());
            transact_log_advance(buf);
            return;
        case type_String: {
            StringData data = value.get_string();
            buf = encode_int(buf, data.size());
            transact_log_advance(buf);
            transact_log_append(data.data(), data.size()); // Throws
            return;
        }
        case type_Binary: {
            BinaryData data = value.get_binary();
            buf = encode_int(buf, data.size());
            transact_log_advance(buf);
            transact_log_append(data.data(), data.size()); // Throws
            return;
        }
        case type_Table:
            transact_log_advance(buf);
            return;
        case type_Mixed:
            break;
    }
    TIGHTDB_ASSERT(false);
}


inline void Replication::new_group_level_table(StringData name)
{
    simple_cmd(instr_NewGroupLevelTable, util::tuple(name.size())); // Throws
    transact_log_append(name.data(), name.size()); // Throws
}


inline void Replication::insert_column(const Descriptor& desc, std::size_t column_ndx,
                                       DataType type, StringData name)
{
    check_desc(desc); // Throws
    simple_cmd(instr_InsertColumn, util::tuple(column_ndx, int(type), name.size())); // Throws
    transact_log_append(name.data(), name.size()); // Throws
}

inline void Replication::erase_column(const Descriptor& desc, std::size_t column_ndx)
{
    check_desc(desc); // Throws
    simple_cmd(instr_EraseColumn, util::tuple(column_ndx)); // Throws
}

inline void Replication::rename_column(const Descriptor& desc, std::size_t column_ndx,
                                       StringData name)
{
    check_desc(desc); // Throws
    simple_cmd(instr_RenameColumn, util::tuple(column_ndx, name.size())); // Throws
    transact_log_append(name.data(), name.size()); // Throws
}


inline void Replication::set_int(const Table* t, std::size_t column_ndx,
                                 std::size_t ndx, int_fast64_t value)
{
    check_table(t); // Throws
    simple_cmd(instr_SetInt, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::set_bool(const Table* t, std::size_t column_ndx,
                                  std::size_t ndx, bool value)
{
    check_table(t); // Throws
    simple_cmd(instr_SetBool, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::set_float(const Table* t, std::size_t column_ndx,
                                   std::size_t ndx, float value)
{
    check_table(t); // Throws
    simple_cmd(instr_SetFloat, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::set_double(const Table* t, std::size_t column_ndx,
                                    std::size_t ndx, double value)
{
    check_table(t); // Throws
    simple_cmd(instr_SetDouble, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::set_string(const Table* t, std::size_t column_ndx,
                                    std::size_t ndx, StringData value)
{
    check_table(t); // Throws
    string_cmd(instr_SetString, column_ndx, ndx, value.data(), value.size()); // Throws
}

inline void Replication::set_binary(const Table* t, std::size_t column_ndx,
                                    std::size_t ndx, BinaryData value)
{
    check_table(t); // Throws
    string_cmd(instr_SetBinary, column_ndx, ndx, value.data(), value.size()); // Throws
}

inline void Replication::set_date_time(const Table* t, std::size_t column_ndx,
                                       std::size_t ndx, DateTime value)
{
    check_table(t); // Throws
    simple_cmd(instr_SetDateTime, util::tuple(column_ndx, ndx, value.get_datetime())); // Throws
}

inline void Replication::set_table(const Table* t, std::size_t column_ndx,
                                   std::size_t ndx)
{
    check_table(t); // Throws
    simple_cmd(instr_SetTable, util::tuple(column_ndx, ndx)); // Throws
}

inline void Replication::set_mixed(const Table* t, std::size_t column_ndx,
                                   std::size_t ndx, const Mixed& value)
{
    check_table(t); // Throws
    mixed_cmd(instr_SetMixed, column_ndx, ndx, value); // Throws
}


inline void Replication::insert_int(const Table* t, std::size_t column_ndx,
                                    std::size_t ndx, int_fast64_t value)
{
    check_table(t); // Throws
    simple_cmd(instr_InsertInt, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::insert_bool(const Table* t, std::size_t column_ndx,
                                     std::size_t ndx, bool value)
{
    check_table(t); // Throws
    simple_cmd(instr_InsertBool, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::insert_float(const Table* t, std::size_t column_ndx,
                                      std::size_t ndx, float value)
{
    check_table(t); // Throws
    simple_cmd(instr_InsertFloat, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::insert_double(const Table* t, std::size_t column_ndx,
                                       std::size_t ndx, double value)
{
    check_table(t); // Throws
    simple_cmd(instr_InsertDouble, util::tuple(column_ndx, ndx, value)); // Throws
}

inline void Replication::insert_string(const Table* t, std::size_t column_ndx,
                                       std::size_t ndx, StringData value)
{
    check_table(t); // Throws
    string_cmd(instr_InsertString, column_ndx, ndx, value.data(), value.size()); // Throws
}

inline void Replication::insert_binary(const Table* t, std::size_t column_ndx,
                                       std::size_t ndx, BinaryData value)
{
    check_table(t); // Throws
    string_cmd(instr_InsertBinary, column_ndx, ndx, value.data(), value.size()); // Throws
}

inline void Replication::insert_date_time(const Table* t, std::size_t column_ndx,
                                          std::size_t ndx, DateTime value)
{
    check_table(t); // Throws
    simple_cmd(instr_InsertDateTime, util::tuple(column_ndx, ndx, value.get_datetime())); // Throws
}

inline void Replication::insert_table(const Table* t, std::size_t column_ndx,
                                      std::size_t ndx)
{
    check_table(t); // Throws
    simple_cmd(instr_InsertTable, util::tuple(column_ndx, ndx)); // Throws
}

inline void Replication::insert_mixed(const Table* t, std::size_t column_ndx,
                                      std::size_t ndx, const Mixed& value)
{
    check_table(t); // Throws
    mixed_cmd(instr_InsertMixed, column_ndx, ndx, value); // Throws
}


inline void Replication::row_insert_complete(const Table* t)
{
    check_table(t); // Throws
    simple_cmd(instr_RowInsertComplete, util::tuple()); // Throws
}


inline void Replication::insert_empty_rows(const Table* t, std::size_t row_ndx,
                                           std::size_t num_rows)
{
    check_table(t); // Throws
    simple_cmd(instr_InsertEmptyRows, util::tuple(row_ndx, num_rows)); // Throws
}


inline void Replication::erase_row(const Table* t, std::size_t row_ndx)
{
    check_table(t); // Throws
    simple_cmd(instr_EraseRow, util::tuple(row_ndx)); // Throws
}


inline void Replication::add_int_to_column(const Table* t, std::size_t column_ndx, int_fast64_t value)
{
    check_table(t); // Throws
    simple_cmd(instr_AddIntToColumn, util::tuple(column_ndx, value)); // Throws
}


inline void Replication::add_index_to_column(const Table* t, std::size_t column_ndx)
{
    check_table(t); // Throws
    simple_cmd(instr_AddIndexToColumn, util::tuple(column_ndx)); // Throws
}


inline void Replication::clear_table(const Table* t)
{
    check_table(t); // Throws
    simple_cmd(instr_ClearTable, util::tuple()); // Throws
}


inline void Replication::optimize_table(const Table* t)
{
    check_table(t); // Throws
    simple_cmd(instr_OptimizeTable, util::tuple()); // Throws
}


inline void Replication::on_table_destroyed(const Table* t) TIGHTDB_NOEXCEPT
{
    if (m_selected_table == t)
        m_selected_table = 0;
}


inline void Replication::on_spec_destroyed(const Spec* s) TIGHTDB_NOEXCEPT
{
    if (m_selected_spec == s)
        m_selected_spec = 0;
}


inline Replication::TransactLogParser::TransactLogParser(Replication::InputStream& transact_log):
    m_input(transact_log)
{
}


inline Replication::TransactLogParser::~TransactLogParser() TIGHTDB_NOEXCEPT
{
}


template<class InstructionHandler>
void Replication::TransactLogParser::parse(InstructionHandler& handler)
{
    if (!m_input_buffer)
        m_input_buffer.reset(new char[m_input_buffer_size]); // Throws
    m_input_begin = m_input_end = m_input_buffer.get();

    DescriptorRef desc;
    for (;;) {
        char instr;
        if (!read_char(instr))
            break;
//cerr << "["<<promote(instr)<<"]";
        switch (Instruction(instr)) {
            case instr_SetInt: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                // FIXME: Don't depend on the existence of int64_t,
                // but don't allow values to use more than 64 bits
                // either.
                int_fast64_t value = read_int<int64_t>(); // Throws
                if (!handler.set_int(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetBool: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                bool value = read_int<bool>(); // Throws
                if (!handler.set_bool(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetFloat: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                float value = read_float(); // Throws
                if (!handler.set_float(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetDouble: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                double value = read_double(); // Throws
                if (!handler.set_double(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetString: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                read_string(m_string_buffer); // Throws
                StringData value(m_string_buffer.data(), m_string_buffer.size());
                if (!handler.set_string(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetBinary: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                read_string(m_string_buffer); // Throws
                BinaryData value(m_string_buffer.data(), m_string_buffer.size());
                if (!handler.set_binary(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetDateTime: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                std::time_t value = read_int<std::time_t>(); // Throws
                if (!handler.set_date_time(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetTable: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                if (!handler.set_table(col_ndx, row_ndx)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SetMixed: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                Mixed value;
                read_mixed(&value); // Throws
                if (!handler.set_mixed(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertInt: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                // FIXME: Don't depend on the existence of int64_t,
                // but don't allow values to use more than 64 bits
                // either.
                int_fast64_t value = read_int<int64_t>(); // Throws
                if (!handler.insert_int(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertBool: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                bool value = read_int<bool>(); // Throws
                if (!handler.insert_bool(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertFloat: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                float value = read_float(); // Throws
                if (!handler.insert_float(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertDouble: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                double value = read_double(); // Throws
                if (!handler.insert_double(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertString: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                read_string(m_string_buffer); // Throws
                StringData value(m_string_buffer.data(), m_string_buffer.size());
                if (!handler.insert_string(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertBinary: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                read_string(m_string_buffer); // Throws
                BinaryData value(m_string_buffer.data(), m_string_buffer.size());
                if (!handler.insert_binary(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertDateTime: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                std::time_t value = read_int<std::time_t>(); // Throws
                if (!handler.insert_date_time(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertTable: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                if (!handler.insert_table(col_ndx, row_ndx)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertMixed: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                Mixed value;
                read_mixed(&value); // Throws
                if (!handler.insert_mixed(col_ndx, row_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_RowInsertComplete: {
                if (!handler.row_insert_complete()) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertEmptyRows: {
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                std::size_t num_rows = read_int<std::size_t>(); // Throws
                if (!handler.insert_empty_rows(row_ndx, num_rows)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_EraseRow: {
                std::size_t row_ndx = read_int<std::size_t>(); // Throws
                if (!handler.erase_row(row_ndx)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_AddIntToColumn: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                // FIXME: Don't depend on the existence of int64_t,
                // but don't allow values to use more than 64 bits
                // either.
                int_fast64_t value = read_int<int64_t>(); // Throws
                if (!handler.add_int_to_column(col_ndx, value)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SelectTable: {
                int levels = read_int<int>(); // Throws
                if (levels < 0 || levels > m_max_levels)
                    goto bad_transact_log;
                m_path.reserve(0, 2*levels); // Throws
                std::size_t* path = m_path.data();
                std::size_t group_level_ndx = read_int<std::size_t>(); // Throws
                for (int i = 0; i != levels; ++i) {
                    std::size_t col_ndx = read_int<std::size_t>(); // Throws
                    std::size_t row_ndx = read_int<std::size_t>(); // Throws
                    path[2*i + 0] = col_ndx;
                    path[2*i + 1] = row_ndx;
                }
                if (!handler.select_table(group_level_ndx, levels, path)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_ClearTable: {
                if (!handler.clear_table()) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_AddIndexToColumn: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                if (!handler.add_index_to_column(col_ndx)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_InsertColumn: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                int type = read_int<int>(); // Throws
                if (!is_valid_column_type(type))
                    goto bad_transact_log;
                read_string(m_string_buffer); // Throws
                StringData name(m_string_buffer.data(), m_string_buffer.size());
                if (!handler.insert_column(col_ndx, DataType(type), name)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_EraseColumn: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                if (!handler.erase_column(col_ndx)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_RenameColumn: {
                std::size_t col_ndx = read_int<std::size_t>(); // Throws
                read_string(m_string_buffer); // Throws
                StringData name(m_string_buffer.data(), m_string_buffer.size());
                if (!handler.rename_column(col_ndx, name)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_SelectDescriptor: {
                int levels = read_int<int>(); // Throws
                if (levels < 0 || levels > m_max_levels)
                    goto bad_transact_log;
                m_path.reserve(0, levels); // Throws
                std::size_t* path = m_path.data();
                for (int i = 0; i != levels; ++i) {
                    std::size_t col_ndx = read_int<std::size_t>(); // Throws
                    path[i] = col_ndx;
                }
                if (!handler.select_descriptor(levels, path)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_NewGroupLevelTable: {
                read_string(m_string_buffer); // Throws
                StringData name(m_string_buffer.data(), m_string_buffer.size());
                if (!handler.new_group_level_table(name)) // Throws
                    goto bad_transact_log;
                continue;
            }
            case instr_OptimizeTable: {
                if (!handler.optimize_table()) // Throws
                    goto bad_transact_log;
                continue;
            }
        }

        goto bad_transact_log;
    }

    return;

  bad_transact_log:
    throw BadTransactLog();
}


template<class T> T Replication::TransactLogParser::read_int()
{
    T value = 0;
    int part;
    const int max_bytes = (std::numeric_limits<T>::digits+1+6)/7;
    for (int i = 0; i != max_bytes; ++i) {
        char c;
        if (!read_char(c))
            goto bad_transact_log;
        part = static_cast<unsigned char>(c);
        if (0xFF < part)
            goto bad_transact_log; // Only the first 8 bits may be used in each byte
        if ((part & 0x80) == 0) {
            T p = part & 0x3F;
            if (util::int_shift_left_with_overflow_detect(p, i*7))
                goto bad_transact_log;
            value |= p;
            break;
        }
        if (i == max_bytes-1)
            goto bad_transact_log; // Too many bytes
        value |= T(part & 0x7F) << (i*7);
    }
    if (part & 0x40) {
        // The real value is negative. Because 'value' is positive at
        // this point, the following negation is guaranteed by C++11
        // to never overflow. See C99+TC3 section 6.2.6.2 paragraph 2.
        value = -value;
        if (util::int_subtract_with_overflow_detect(value, 1))
            goto bad_transact_log;
    }
    return value;

  bad_transact_log:
    throw BadTransactLog();
}


inline void Replication::TransactLogParser::read_bytes(char* data, std::size_t size)
{
    for (;;) {
        const std::size_t avail = m_input_end - m_input_begin;
        if (size <= avail)
            break;
        const char* to = m_input_begin + avail;
        std::copy(m_input_begin, to, data);
        if (!fill_input_buffer())
            throw BadTransactLog();
        data += avail;
        size -= avail;
    }
    const char* to = m_input_begin + size;
    std::copy(m_input_begin, to, data);
    m_input_begin = to;
}


inline float Replication::TransactLogParser::read_float()
{
    TIGHTDB_STATIC_ASSERT(std::numeric_limits<float>::is_iec559 &&
                          sizeof (float) * std::numeric_limits<unsigned char>::digits == 32,
                          "Unsupported 'float' representation");
    float value;
    read_bytes(reinterpret_cast<char*>(&value), sizeof value); // Throws
    return value;
}


inline double Replication::TransactLogParser::read_double()
{
    TIGHTDB_STATIC_ASSERT(std::numeric_limits<double>::is_iec559 &&
                          sizeof (double) * std::numeric_limits<unsigned char>::digits == 64,
                          "Unsupported 'double' representation");
    double value;
    read_bytes(reinterpret_cast<char*>(&value), sizeof value); // Throws
    return value;
}


inline void Replication::TransactLogParser::read_string(util::StringBuffer& buf)
{
    buf.clear();
    std::size_t size = read_int<std::size_t>(); // Throws
    buf.resize(size); // Throws
    read_bytes(buf.data(), size);
}


inline void Replication::TransactLogParser::read_mixed(Mixed* mixed)
{
    DataType type = DataType(read_int<int>()); // Throws
    switch (type) {
        case type_Int: {
            // FIXME: Don't depend on the existence of
            // int64_t, but don't allow values to use more
            // than 64 bits either.
            int_fast64_t value = read_int<int64_t>(); // Throws
            mixed->set_int(value);
            return;
        }
        case type_Bool: {
            bool value = read_int<bool>(); // Throws
            mixed->set_bool(value);
            return;
        }
        case type_Float: {
            float value = read_float(); // Throws
            mixed->set_float(value);
            return;
        }
        case type_Double: {
            double value = read_double(); // Throws
            mixed->set_double(value);
            return;
        }
        case type_DateTime: {
            std::time_t value = read_int<std::time_t>(); // Throws
            mixed->set_datetime(value);
            return;
        }
        case type_String: {
            read_string(m_string_buffer); // Throws
            StringData value(m_string_buffer.data(), m_string_buffer.size());
            mixed->set_string(value);
            return;
        }
        case type_Binary: {
            read_string(m_string_buffer); // Throws
            BinaryData value(m_string_buffer.data(), m_string_buffer.size());
            mixed->set_binary(value);
            return;
        }
        case type_Table: {
            *mixed = Mixed::subtable_tag();
            return;
        }
        case type_Mixed:
            break;
    }
    TIGHTDB_ASSERT(false);
}


inline bool Replication::TransactLogParser::fill_input_buffer()
{
    const std::size_t n = m_input.read(m_input_buffer.get(), m_input_buffer_size);
    if (n == 0)
        return false;
    m_input_begin = m_input_buffer.get();
    m_input_end   = m_input_begin + n;
    return true;
}


inline bool Replication::TransactLogParser::read_char(char& c)
{
    if (m_input_begin == m_input_end && !fill_input_buffer())
        return false;
    c = *m_input_begin++;
    return true;
}


inline bool Replication::TransactLogParser::is_valid_column_type(int type)
{
    switch (type) {
        case type_Int:
        case type_Bool:
        case type_Float:
        case type_Double:
        case type_String:
        case type_Binary:
        case type_DateTime:
        case type_Table:
        case type_Mixed:
            return true;
    }
    return false;
}


inline TrivialReplication::TrivialReplication(const std::string& database_file):
    m_database_file(database_file)
{
}

inline void TrivialReplication::transact_log_reserve(std::size_t n)
{
    char* data = m_transact_log_buffer.data();
    std::size_t size = m_transact_log_free_begin - data;
    m_transact_log_buffer.reserve_extra(size, n);
    data = m_transact_log_buffer.data(); // May have changed
    m_transact_log_free_begin = data + size;
    m_transact_log_free_end = data + m_transact_log_buffer.size();
}


} // namespace tightdb

#endif // TIGHTDB_REPLICATION_HPP
