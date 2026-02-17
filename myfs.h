#pragma once

#include "blkdev.h"
#include <stdint.h>
#include <string.h>
#include <string>
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

  auto format(void) -> void;
  auto fileExists(const std::string &path_str) -> bool;
  auto create_file(const std::string &path_str, bool directory) -> void;
  auto get_content(const std::string &path_str) -> std::string;
  auto set_content(const std::string &path_str, const std::string &content) -> void;
  auto list_dir(const std::string &path_str) -> dir_list;

private:
  inline constexpr static int BLOCK_SIZE = 256;
  inline constexpr static int TOTAL_BLOCKS = 16;
  inline constexpr static int DATA_PER_BLOCK = BLOCK_SIZE - 1; // 1 byte for `used` flag
  inline constexpr static int MAX_CONTENT_BLOCKS = 8;
  inline constexpr static int FILE_NAME_MAX_LEN = 10;
  inline constexpr static const char *MYFS_MAGIC = "MYFS";
  inline constexpr static int CURR_VERSION = 5;

  inline constexpr static uint8_t BLOCK_TYPE_INODE = 1;
  inline constexpr static uint8_t BLOCK_TYPE_CONTENT = 2;
  inline constexpr static uint8_t BLOCK_TYPE_UNUSED = 0;

  struct myfs_header
  {
    char magic[4];
    uint8_t version;
  };

  struct InodeBlock
  {
    uint8_t type; // BLOCK_TYPE_INODE
    char name[FILE_NAME_MAX_LEN + 1];
    int size;
    int num_blocks;
    int blocks[MAX_CONTENT_BLOCKS];
  };

  struct ContentBlock
  {
    uint8_t type; // BLOCK_TYPE_CONTENT
    char data[DATA_PER_BLOCK];
  };

  // This is constexpr so i put it in h file.
  constexpr auto blockAddr(int i) const -> int
  {
    return i * BLOCK_SIZE;
  }
  auto findFreeBlock() -> int;

  BlockDeviceSimulator *blkdevsim;
};
