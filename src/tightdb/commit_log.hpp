/*************************************************************************
 *
 * TIGHTDB CONFIDENTIAL
 * __________________
 *
 *  [2011] - [2014] TightDB Inc
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
#ifndef TIGHTDB_COMMIT_LOG_HPP
#define TIGHTDB_COMMIT_LOG_HPP

#include <exception>

#include <tightdb/replication.hpp>
#include <tightdb/binary_data.hpp>
#include <tightdb/group_shared.hpp>

namespace tightdb {


// Obtain the TransactLogRegistry for a specific filepath. Create it, if it doesn't exist.
// The library retains ownership (and thus responsibility for deallocation) of the Registry.
// Deallocation will happen at application termination.
SharedGroup::TransactLogRegistry* getWriteLogs(std::string filepath);

// Create a writelog collector and associate it with a filepath. You'll need one writelog
// collector for each shared group. Commits from writelog collectors for a specific filepath 
// may later be obtained through the TransactLogRegistry associated with said filepath.
// The caller assumes ownership of the writelog collector and must destroy it, but only AFTER
// destruction of the shared group using it.
Replication* makeWriteLogCollector(std::string filepath);


} // namespace tightdb

#endif // TIGHTDB_COMMIT_LOG_HPP
