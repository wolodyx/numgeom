#include "downloaddata.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "httplib.h"
#include "zlib.h"

namespace {

//! Read a little-endian uint16 from a byte buffer.
uint16_t ReadLE16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

//! Read a little-endian uint32 from a byte buffer.
uint32_t ReadLE32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

//! ZIP signatures.
constexpr uint32_t kLocalFileHeaderSig = 0x04034b50;
constexpr uint32_t kCentralDirHeaderSig = 0x02014b50;
constexpr uint32_t kEocdSig = 0x06054b50;

//! Minimum sizes of ZIP structures (fixed fields only).
constexpr int kLocalFileHeaderFixedSize = 30;
constexpr int kCentralDirFixedSize = 46;
constexpr int kEocdFixedSize = 22;

//! End of Central Directory record
struct Eocd {
  uint16_t disk_num;
  uint16_t disk_start;
  uint16_t num_entries_on_disk;
  uint16_t num_entries_total;
  uint32_t central_dir_size;
  uint32_t central_dir_offset;
};

//! Locate the End of Central Directory signature by scanning backwards
//! from the end of the archive (the comment can be up to 65535 bytes).
bool FindEocd(const std::vector<uint8_t>& archive, Eocd& eocd) {
  if (archive.size() < kEocdFixedSize) return false;

  // Maximum comment length is 65535, so search the last 65535 + 22 bytes.
  const size_t search_start =
      (archive.size() > 65535 + kEocdFixedSize)
          ? archive.size() - 65535 - kEocdFixedSize
          : 0;

  for (size_t i = archive.size() - kEocdFixedSize; i >= search_start; --i) {
    if (ReadLE32(archive.data() + i) == kEocdSig) {
      const uint8_t* p = archive.data() + i;
      eocd.disk_num = ReadLE16(p + 4);
      eocd.disk_start = ReadLE16(p + 6);
      eocd.num_entries_on_disk = ReadLE16(p + 8);
      eocd.num_entries_total = ReadLE16(p + 10);
      eocd.central_dir_size = ReadLE32(p + 12);
      eocd.central_dir_offset = ReadLE32(p + 16);
      return true;
    }
    if (i == 0) break;  // prevent underflow on size_t
  }
  return false;
}

//! Central Directory entry
struct CentralDirEntry {
  uint16_t compression;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  uint16_t name_len;
  uint16_t extra_len;
  uint16_t comment_len;
  uint32_t local_header_offset;
  std::string filename;
};

//! Read one central directory entry at \p offset.
//! Returns false if the signature doesn't match or the entry is truncated.
bool ReadCentralDirEntry(const std::vector<uint8_t>& archive, size_t offset,
                         CentralDirEntry& entry) {
  if (offset + kCentralDirFixedSize > archive.size()) return false;
  const uint8_t* p = archive.data() + offset;
  if (ReadLE32(p) != kCentralDirHeaderSig) return false;

  entry.compression = ReadLE16(p + 10);
  entry.compressed_size = ReadLE32(p + 20);
  entry.uncompressed_size = ReadLE32(p + 24);
  entry.name_len = ReadLE16(p + 28);
  entry.extra_len = ReadLE16(p + 30);
  entry.comment_len = ReadLE16(p + 32);
  entry.local_header_offset = ReadLE32(p + 42);

  if (offset + kCentralDirFixedSize + entry.name_len > archive.size())
    return false;
  const uint8_t* name_ptr = p + kCentralDirFixedSize;
  entry.filename.assign(name_ptr, name_ptr + entry.name_len);
  return true;
}

//! Inflate (decompress) a single deflate block
bool InflateBlock(const std::vector<uint8_t>& compressed,
                  std::vector<uint8_t>& decompressed) {
  constexpr size_t kChunkSize = 65536;  // 64 KiB per chunk
  decompressed.resize(kChunkSize);

  z_stream strm{};
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = static_cast<uInt>(compressed.size());
  strm.next_in =
      const_cast<uint8_t*>(compressed.data());  // NOLINT: zlib API quirk
  strm.avail_out = static_cast<uInt>(decompressed.size());
  strm.next_out = decompressed.data();

  // Raw inflate (no zlib/gzip header) – ZIP uses raw deflate.
  int ret = inflateInit2(&strm, -MAX_WBITS);
  if (ret != Z_OK) {
    std::cerr << "InflateBlock: inflateInit2 failed (" << ret << ")\n";
    return false;
  }

  do {
    ret = inflate(&strm, Z_NO_FLUSH);
    if (ret == Z_STREAM_END) break;
    if (ret != Z_OK) {
      std::cerr << "InflateBlock: inflate failed (" << ret << ")\n";
      (void)inflateEnd(&strm);
      return false;
    }
    size_t pos = decompressed.size();
    decompressed.resize(pos + kChunkSize);
    strm.avail_out = static_cast<uInt>(decompressed.size() - pos);
    strm.next_out = decompressed.data() + pos;
  } while (true);

  (void)inflateEnd(&strm);
  decompressed.resize(decompressed.size() - strm.avail_out);
  return true;
}

//! Extract a single entry using sizes from the central directory
bool ExtractZipEntry(const std::vector<uint8_t>& archive,
                     const CentralDirEntry& cd_entry,
                     const std::filesystem::path& dir) {
  const std::string& filename = cd_entry.filename;

  // Skip directories.
  if (filename.empty() || filename.back() == '/' ||
      filename.back() == '\\') {
    return true;
  }
  // Path safety.
  if (filename.find("..") != std::string::npos) {
    std::cerr << "ExtractZipEntry: skipping unsafe path '" << filename
              << "'\n";
    return true;
  }

  // Locate the local file header.
  size_t local_off = cd_entry.local_header_offset;
  if (local_off + kLocalFileHeaderFixedSize > archive.size()) return false;
  if (ReadLE32(archive.data() + local_off) != kLocalFileHeaderSig)
    return false;

  const uint8_t* lh = archive.data() + local_off;
  const uint16_t name_len = ReadLE16(lh + 26);
  const uint16_t extra_len = ReadLE16(lh + 28);

  const size_t data_off = local_off + kLocalFileHeaderFixedSize +
                          name_len + extra_len;
  const uint8_t* data_ptr = archive.data() + data_off;

  if (data_off + cd_entry.compressed_size > archive.size()) return false;

  // Build output path.
  std::filesystem::path out_path = dir / filename;
  std::error_code ec;
  std::filesystem::create_directories(out_path.parent_path(), ec);

  std::vector<uint8_t> out_data;

  if (cd_entry.compression == 0) {
    // Stored.
    out_data.assign(data_ptr, data_ptr + cd_entry.compressed_size);
  } else if (cd_entry.compression == 8) {
    // Deflated.
    if (!InflateBlock({data_ptr, data_ptr + cd_entry.compressed_size},
                      out_data)) {
      return false;
    }
  } else {
    std::cerr << "ExtractZipEntry: unsupported compression method "
              << cd_entry.compression << " for '" << filename << "'\n";
    return false;
  }

  std::ofstream ofs(out_path, std::ios::binary);
  if (!ofs) {
    std::cerr << "ExtractZipEntry: cannot write '" << out_path << "'\n";
    return false;
  }
  ofs.write(reinterpret_cast<const char*>(out_data.data()),
            static_cast<std::streamsize>(out_data.size()));
  return true;
}

//! Main extraction: parse central directory, then extract each entry
bool ExtractZip(const std::vector<uint8_t>& archive,
                const std::filesystem::path& dir) {
  // 1. Locate the End of Central Directory.
  Eocd eocd{};
  if (!FindEocd(archive, eocd)) {
    std::cerr << "ExtractZip: cannot find End of Central Directory\n";
    return false;
  }

  if (eocd.num_entries_total == 0) {
    std::cerr << "ExtractZip: archive is empty\n";
    return false;
  }

  // 2. Iterate over central directory entries.
  size_t cd_offset = eocd.central_dir_offset;
  bool any_extracted = false;

  for (uint16_t i = 0; i < eocd.num_entries_total; ++i) {
    CentralDirEntry cd_entry{};
    if (!ReadCentralDirEntry(archive, cd_offset, cd_entry)) {
      std::cerr << "ExtractZip: failed to read central dir entry " << i
                << " at offset " << cd_offset << "\n";
      return false;
    }

    if (!ExtractZipEntry(archive, cd_entry, dir)) {
      std::cerr << "ExtractZip: failed to extract '" << cd_entry.filename
                << "'\n";
      return false;
    }
    any_extracted = true;

    cd_offset += kCentralDirFixedSize + cd_entry.name_len +
                 cd_entry.extra_len + cd_entry.comment_len;
  }

  if (!any_extracted) {
    std::cerr << "ExtractZip: no entries extracted\n";
    return false;
  }
  return true;
}

//! URL parsing (minimal – extracts host and path from an http/https URL)
struct ParsedUrl {
  std::string host;
  std::string path;
  bool is_https;
};

bool ParseUrl(const std::string& url, ParsedUrl& parsed) {
  // Determine protocol.
  std::string remaining;
  if (url.compare(0, 8, "https://") == 0) {
    parsed.is_https = true;
    remaining = url.substr(8);
  } else if (url.compare(0, 7, "http://") == 0) {
    parsed.is_https = false;
    remaining = url.substr(7);
  } else {
    std::cerr << "ParseUrl: unsupported protocol in '" << url << "'\n";
    return false;
  }
  // Split host and path at the first '/'.
  auto slash_pos = remaining.find('/');
  if (slash_pos == std::string::npos) {
    parsed.host = remaining;
    parsed.path = "/";
  } else {
    parsed.host = remaining.substr(0, slash_pos);
    parsed.path = remaining.substr(slash_pos);
  }
  // Strip port suffix if present (not needed for httplib::Client).
  auto colon_pos = parsed.host.find(':');
  if (colon_pos != std::string::npos) {
    parsed.host = parsed.host.substr(0, colon_pos);
  }
  return true;
}

}  // anonymous namespace

// ===========================================================================

bool DownloadData(const std::string& url, const std::filesystem::path& dir) {
  ParsedUrl parsed;
  if (!ParseUrl(url, parsed)) {
    std::cerr << "DownloadData: failed to parse URL '" << url << "'\n";
    return false;
  }

  // Create the target directory if it doesn't exist.
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);

  // Set up the HTTP client.
  httplib::Client cli(parsed.host);
  cli.set_follow_location(true);
  cli.set_connection_timeout(30);
  cli.set_read_timeout(60);

  auto res = cli.Get(parsed.path);
  if (!res) {
    std::cerr << "DownloadData: connection error for '" << url << "'\n";
    return false;
  }
  if (res->status != 200) {
    std::cerr << "DownloadData: HTTP " << res->status << " for '" << url
              << "'\n";
    return false;
  }

  // The response body is the raw ZIP archive.
  const std::string& body = res->body;
  if (body.empty()) {
    std::cerr << "DownloadData: empty response body\n";
    return false;
  }

  std::vector<uint8_t> archive(body.begin(), body.end());

  // Extract the ZIP archive into the target directory.
  if (!ExtractZip(archive, dir)) {
    std::cerr << "DownloadData: failed to extract archive into '"
              << dir << "'\n";
    return false;
  }

  return true;
}
