#pragma once

#include "blkdev.h"
#include <memory>
#include <stdint.h>
#include <string.h>
#include <vector>

class MyFs
{
public:
  MyFs(BlockDeviceSimulator *blkdevsim_);
  ~MyFs(void) = default;

  struct dir_list_entry
  {
    std::string name;
    bool is_dir;
    int file_size;
  };
  typedef std::vector<struct dir_list_entry> dir_list;

  void format(void);
  void create_file(const std::string &path_str, bool directory);
  std::string get_content(const std::string &path_str);
  void set_content(const std::string &path_str, const std::string &content);
  dir_list list_dir(const std::string &path_str);

private:
  inline constexpr static int FILE_NAME_MAX_LENGTH = 10;
  inline constexpr static int MAX_FILES = 64;      // Max number of files
  inline constexpr static int FILE_MAX_SIZE = 256; // block size (changable)
  inline constexpr static const char *MYFS_MAGIC = "MYFS";
  inline constexpr static int CURR_VERSION = 3;

  struct myfs_header
  {
    char magic[4];
    uint8_t version;
  };

  struct FileEntry
  {
    char name[FILE_NAME_MAX_LENGTH + 1];
    int size;
    int start_address; // address on the block device
    bool is_used;
  };

  inline constexpr static int METADATA_OFFSET = sizeof(struct myfs_header);                          // Start of metadata table (after header)
  inline constexpr static int DATA_START_OFFSET = METADATA_OFFSET + (sizeof(FileEntry) * MAX_FILES); // After ENTTIRE! metadata table

  BlockDeviceSimulator *blkdevsim;
};
