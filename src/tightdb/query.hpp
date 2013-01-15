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
#ifndef TIGHTDB_QUERY_HPP
#define TIGHTDB_QUERY_HPP

#include <string>
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <limits.h>
#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
    #include <win32/pthread/pthread.h>
    #include <win32/stdint.h>
#else
    #include <pthread.h>
#endif

#include <tightdb/table_ref.hpp>
#include <tightdb/binary_data.hpp>

namespace tightdb {


// Pre-declarations
class ParentNode;
class Table;
class TableView;
class ConstTableView;
class Array;

const size_t MAX_THREADS = 128;

class Query {
public:
    Query(const Query& copy); // FIXME: Try to remove this
    ~Query();

    // Conditions: Query only rows contained in tv
    Query& tableview(const TableView& tv);
    Query& tableview(const Array& arr);

    // Conditions: int
    Query& equal(size_t column_ndx, int64_t value);
    Query& not_equal(size_t column_ndx, int64_t value);
    Query& greater(size_t column_ndx, int64_t value);
    Query& greater_equal(size_t column_ndx, int64_t value);
    Query& less(size_t column_ndx, int64_t value);
    Query& less_equal(size_t column_ndx, int64_t value);
    Query& between(size_t column_ndx, int64_t from, int64_t to);

    // Conditions: float
    Query& equal(size_t column_ndx, float value);
    Query& not_equal(size_t column_ndx, float value);
    Query& greater(size_t column_ndx, float value);
    Query& greater_equal(size_t column_ndx, float value);
    Query& less(size_t column_ndx, float value);
    Query& less_equal(size_t column_ndx, float value);
    Query& between(size_t column_ndx, float from, float to);

     // Conditions: double
    Query& equal(size_t column_ndx, double value);
    Query& not_equal(size_t column_ndx, double value);
    Query& greater(size_t column_ndx, double value);
    Query& greater_equal(size_t column_ndx, double value);
    Query& less(size_t column_ndx, double value);
    Query& less_equal(size_t column_ndx, double value);
    Query& between(size_t column_ndx, double from, double to);

    // Conditions: bool
    Query& equal(size_t column_ndx, bool value);

    // Conditions: strings
    Query& equal(size_t column_ndx, const char* value, bool caseSensitive=true);
    Query& begins_with(size_t column_ndx, const char* value, bool caseSensitive=true);
    Query& ends_with(size_t column_ndx, const char* value, bool caseSensitive=true);
    Query& contains(size_t column_ndx, const char* value, bool caseSensitive=true);
    Query& not_equal(size_t column_ndx, const char* value, bool caseSensitive=true);

    // Conditions: date
    // FIXME: Maybe we can just use 'int' versions for date, but why then do we have a special 'date' column type?
    // FIXME: The '_date' suffix is needed because 'time_t' may not be distinguishable from 'int64_t' on all platforms.
/*
    Query& equal_date(size_t column_ndx, time_t value);
    Query& not_equal_date(size_t column_ndx, time_t value);
    Query& greater_date(size_t column_ndx, time_t value);
    Query& greater_equal_date(size_t column_ndx, time_t value);
    Query& less_date(size_t column_ndx, time_t value);
    Query& less_equal_date(size_t column_ndx, time_t value);
    Query& between_date(size_t column_ndx, time_t from, time_t to);
*/

    // Conditions: binary data
    // Only BinaryData prototype can exist, else it would conflict with equal() for strings 

    Query& equal(size_t column_ndx, BinaryData b);
/*
    Query& equal_binary(size_t column_ndx, const char* ptr, size_t len);
    Query& begins_with_binary(size_t column_ndx, const char* ptr, size_t len);
    Query& ends_with_binary(size_t column_ndx, const char* ptr, size_t len);
    Query& contains_binary(size_t column_ndx, const char* ptr, size_t len);
*/

    // Grouping
    Query& group();
    Query& end_group();
    void subtable(size_t column);
    void end_subtable();
    Query& Or();

    // Searching
    size_t         find_next(size_t lastmatch=-1);
    TableView      find_all(size_t start=0, size_t end=size_t(-1), size_t limit=size_t(-1));
    ConstTableView find_all(size_t start=0, size_t end=size_t(-1), size_t limit=size_t(-1)) const;

    // Aggregates
    int64_t sum(size_t column, size_t* resultcount=NULL, size_t start=0, size_t end = size_t(-1), size_t limit=size_t(-1)) const;
    int64_t maximum(size_t column, size_t* resultcount=NULL, size_t start=0, size_t end = size_t(-1), size_t limit=size_t(-1)) const;
    int64_t minimum(size_t column, size_t* resultcount=NULL, size_t start=0, size_t end = size_t(-1), size_t limit=size_t(-1)) const;
    double  average(size_t column_ndx, size_t* resultcount=NULL, size_t start=0, size_t end=size_t(-1), size_t limit=size_t(-1)) const;
    size_t  count(size_t start=0, size_t end=size_t(-1), size_t limit=size_t(-1)) const;
/*
    time_t maximum_date(const Table& table, size_t column, size_t* resultcount=NULL, size_t start=0, size_t end = size_t(-1), size_t limit=size_t(-1)) const;
    time_t minimum_date(const Table& table, size_t column, size_t* resultcount=NULL, size_t start=0, size_t end = size_t(-1), size_t limit=size_t(-1)) const;
*/
    float sum_float(size_t column, size_t* resultcount=NULL, size_t start=0, size_t end = size_t(-1), size_t limit=size_t(-1)) const;

    // Deletion
    size_t  remove(size_t start=0, size_t end=size_t(-1), size_t limit=size_t(-1));

    // Multi-threading
    TableView      find_all_multi(size_t start=0, size_t end=size_t(-1));
    ConstTableView find_all_multi(size_t start=0, size_t end=size_t(-1)) const;
    int            set_threads(unsigned int threadcount);

    TableRef& get_table() {return m_table;}

#ifdef TIGHTDB_DEBUG
    std::string Verify(); // Must be upper case to avoid conflict with macro in ObjC
#endif

    std::string error_code;

protected:
    friend class Table;
    template <typename T> friend class BasicTable;
    friend class XQueryAccessorInt;
    friend class XQueryAccessorString;

    Query(Table& table);
    Query(const Table& table); // FIXME: This constructor should not exist. We need a ConstQuery class.
    void Create();

    void   Init(const Table& table) const;
    size_t FindInternal(size_t start=0, size_t end=size_t(-1)) const;
    void   UpdatePointers(ParentNode* p, ParentNode** newnode);

    static bool  comp(const std::pair<size_t, size_t>& a, const std::pair<size_t, size_t>& b);
    static void* query_thread(void* arg);

    struct thread_state {
        pthread_mutex_t result_mutex;
        pthread_cond_t  completed_cond;
        pthread_mutex_t completed_mutex;
        pthread_mutex_t jobs_mutex;
        pthread_cond_t  jobs_cond;
        size_t next_job;
        size_t end_job;
        size_t done_job;
        size_t count;
        ParentNode* node;
        Table* table;
        std::vector<size_t> results;
        std::vector<std::pair<size_t, size_t> > chunks;
    } ts;
    pthread_t threads[MAX_THREADS];

    TableRef m_table;
    std::vector<ParentNode*> first;
    std::vector<ParentNode**> update;
    std::vector<ParentNode**> update_override;
    std::vector<ParentNode**> subtables;
    std::vector<ParentNode*> all_nodes;
    mutable bool do_delete;
private:
    size_t m_threadcount;

    template <typename T, class N>
    Query& add_condition(size_t column_ndx, T value);

};


} // namespace tightdb

#endif // TIGHTDB_QUERY_HPP
