#include "library/library_manager.h"
#include "diagnostics/logger.h"
#include "utils/string_utils.h"
#include "utils/file_utils.h"
#include <algorithm>

#ifdef DOCVISION_HAS_SQLITE
#include <sqlite3.h>
#endif

namespace docvision {

LibraryManager::LibraryManager() {}

LibraryManager::~LibraryManager() {
    close();
}

bool LibraryManager::initialize(const std::wstring& dbPath) {
    m_dbPath = dbPath;
#ifdef DOCVISION_HAS_SQLITE
    sqlite3* db = nullptr;
    std::string path8 = utils::wideToUtf8(dbPath);
    if (sqlite3_open(path8.c_str(), &db) != SQLITE_OK) {
        LOG_ERROR("Failed to open library database: " + path8);
        return false;
    }
    m_db = db;
    return createSchema();
#else
    LOG_WARNING("Library built without SQLite support, using in-memory storage");
    return true;
#endif
}

void LibraryManager::close() {
#ifdef DOCVISION_HAS_SQLITE
    if (m_db) {
        sqlite3_close(static_cast<sqlite3*>(m_db));
        m_db = nullptr;
    }
#endif
}

bool LibraryManager::createSchema() {
#ifdef DOCVISION_HAS_SQLITE
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS library (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT UNIQUE NOT NULL,
            title TEXT DEFAULT '',
            author TEXT DEFAULT '',
            format TEXT DEFAULT '',
            page_count INTEGER DEFAULT 0,
            file_size INTEGER DEFAULT 0,
            added_date TEXT DEFAULT (datetime('now')),
            last_opened_date TEXT DEFAULT '',
            last_page INTEGER DEFAULT 0,
            last_zoom REAL DEFAULT 1.0,
            is_pinned INTEGER DEFAULT 0,
            collection TEXT DEFAULT '',
            content_hash TEXT DEFAULT ''
        );
        CREATE TABLE IF NOT EXISTS tags (
            entry_id INTEGER NOT NULL,
            tag TEXT NOT NULL,
            PRIMARY KEY (entry_id, tag),
            FOREIGN KEY (entry_id) REFERENCES library(id) ON DELETE CASCADE
        );
        CREATE INDEX IF NOT EXISTS idx_library_path ON library(file_path);
        CREATE INDEX IF NOT EXISTS idx_library_last_opened ON library(last_opened_date);
    )";
    sqlite3* db = static_cast<sqlite3*>(m_db);
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        LOG_ERROR(std::string("Failed to create schema: ") + (errMsg ? errMsg : "unknown"));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
#else
    return true;
#endif
}

int64_t LibraryManager::addEntry(const LibraryEntry& entry) {
#ifdef DOCVISION_HAS_SQLITE
    if (!m_db) return -1;
    sqlite3* db = static_cast<sqlite3*>(m_db);
    const char* sql = "INSERT OR REPLACE INTO library "
        "(file_path, title, author, format, page_count, file_size, last_page, last_zoom, is_pinned, collection, content_hash) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    std::string path8 = utils::wideToUtf8(entry.filePath);
    std::string title8 = utils::wideToUtf8(entry.title);
    std::string author8 = utils::wideToUtf8(entry.author);
    std::string format8 = utils::wideToUtf8(entry.format);
    std::string collection8 = utils::wideToUtf8(entry.collection);
    std::string hash8 = entry.contentHash.empty() ? "" : utils::wideToUtf8(entry.contentHash);

    sqlite3_bind_text(stmt, 1, path8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, title8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, author8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, format8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, entry.pageCount);
    sqlite3_bind_int64(stmt, 6, entry.fileSize);
    sqlite3_bind_int(stmt, 7, entry.lastPage);
    sqlite3_bind_double(stmt, 8, entry.lastZoom);
    sqlite3_bind_int(stmt, 9, entry.isPinned ? 1 : 0);
    sqlite3_bind_text(stmt, 10, collection8.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, hash8.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    int64_t id = (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(db) : -1;
    sqlite3_finalize(stmt);
    return id;
#else
    (void)entry;
    return -1;
#endif
}

bool LibraryManager::updateEntry(const LibraryEntry& entry) {
    return addEntry(entry) >= 0;
}

bool LibraryManager::removeEntry(int64_t id) {
#ifdef DOCVISION_HAS_SQLITE
    if (!m_db) return false;
    sqlite3* db = static_cast<sqlite3*>(m_db);
    const char* sql = "DELETE FROM library WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
#else
    (void)id;
    return false;
#endif
}

bool LibraryManager::removeByPath(const std::wstring& path) {
#ifdef DOCVISION_HAS_SQLITE
    if (!m_db) return false;
    sqlite3* db = static_cast<sqlite3*>(m_db);
    const char* sql = "DELETE FROM library WHERE file_path = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    std::string path8 = utils::wideToUtf8(path);
    sqlite3_bind_text(stmt, 1, path8.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
#else
    (void)path;
    return false;
#endif
}

LibraryEntry LibraryManager::getEntry(int64_t id) const {
    (void)id;
    return {};
}

LibraryEntry LibraryManager::getEntryByPath(const std::wstring& path) const {
    (void)path;
    return {};
}

std::vector<LibraryEntry> LibraryManager::query(const LibraryFilter& filter) const {
    (void)filter;
    return {};
}

int LibraryManager::entryCount() const {
    return 0;
}

bool LibraryManager::hasEntry(const std::wstring& path) const {
    (void)path;
    return false;
}

std::vector<LibraryEntry> LibraryManager::getRecentFiles(int limit) const {
    (void)limit;
    return {};
}

void LibraryManager::updateLastOpened(const std::wstring& path, int page, double zoom) {
    (void)path; (void)page; (void)zoom;
}

void LibraryManager::setPinned(const std::wstring& path, bool pinned) {
    (void)path; (void)pinned;
}

std::vector<LibraryEntry> LibraryManager::getPinnedFiles() const {
    return {};
}

void LibraryManager::setCollection(int64_t id, const std::wstring& collection) {
    (void)id; (void)collection;
}

std::vector<std::wstring> LibraryManager::getCollections() const {
    return {};
}

void LibraryManager::addTag(int64_t id, const std::wstring& tag) {
    (void)id; (void)tag;
}

void LibraryManager::removeTag(int64_t id, const std::wstring& tag) {
    (void)id; (void)tag;
}

std::vector<std::wstring> LibraryManager::getAllTags() const {
    return {};
}

int LibraryManager::importFolder(const std::wstring& folderPath, bool recursive) {
    (void)folderPath; (void)recursive;
    return 0;
}

int LibraryManager::cleanupMissing() {
    return 0;
}

void LibraryManager::clearAll() {}
void LibraryManager::clearRecent() {}

} // namespace docvision
