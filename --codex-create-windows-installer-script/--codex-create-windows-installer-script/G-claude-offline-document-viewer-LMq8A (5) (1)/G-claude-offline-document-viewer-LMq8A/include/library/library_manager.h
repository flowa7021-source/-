#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace docvision {

// Library entry
struct LibraryEntry {
    int64_t id = 0;
    std::wstring filePath;
    std::wstring title;
    std::wstring author;
    std::wstring format;        // "PDF", "DjVu", "CBZ", "EPUB"
    int pageCount = 0;
    int64_t fileSize = 0;
    std::wstring addedDate;
    std::wstring lastOpenedDate;
    int lastPage = 0;           // reading position
    double lastZoom = 1.0;
    bool isPinned = false;
    std::vector<std::wstring> tags;
    std::wstring collection;
    std::wstring contentHash;
};

// Library filter
struct LibraryFilter {
    std::wstring searchQuery;
    std::wstring format;        // empty = all formats
    std::wstring collection;
    std::wstring tag;
    enum class SortBy { Title, DateAdded, DateOpened, Format, FileSize } sortBy = SortBy::DateOpened;
    bool sortAscending = false;
    int limit = 0;              // 0 = no limit
    int offset = 0;
};

// Library manager — document catalog stored in portable SQLite database
class LibraryManager {
public:
    LibraryManager();
    ~LibraryManager();

    // Initialize with database path
    bool initialize(const std::wstring& dbPath);
    void close();

    // CRUD
    int64_t addEntry(const LibraryEntry& entry);
    bool updateEntry(const LibraryEntry& entry);
    bool removeEntry(int64_t id);
    bool removeByPath(const std::wstring& path);

    // Query
    LibraryEntry getEntry(int64_t id) const;
    LibraryEntry getEntryByPath(const std::wstring& path) const;
    std::vector<LibraryEntry> query(const LibraryFilter& filter = {}) const;
    int entryCount() const;
    bool hasEntry(const std::wstring& path) const;

    // Recent files
    std::vector<LibraryEntry> getRecentFiles(int limit = 20) const;
    void updateLastOpened(const std::wstring& path, int page, double zoom);

    // Pinned
    void setPinned(const std::wstring& path, bool pinned);
    std::vector<LibraryEntry> getPinnedFiles() const;

    // Collections
    void setCollection(int64_t id, const std::wstring& collection);
    std::vector<std::wstring> getCollections() const;

    // Tags
    void addTag(int64_t id, const std::wstring& tag);
    void removeTag(int64_t id, const std::wstring& tag);
    std::vector<std::wstring> getAllTags() const;

    // Folder import (scan folder and add all supported files)
    int importFolder(const std::wstring& folderPath, bool recursive = true);

    // Cleanup: remove entries for deleted files
    int cleanupMissing();

    // Clear history
    void clearAll();
    void clearRecent(); // remove entries that aren't pinned

private:
    bool createSchema();
    void* m_db = nullptr; // sqlite3*
    std::wstring m_dbPath;
};

} // namespace docvision
