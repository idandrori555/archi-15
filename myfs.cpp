// <!> USE C++23 <!>

#include "myfs.h"
#include <iostream>
#include <stdexcept>
#include <string.h>

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_) : blkdevsim(blkdevsim_)
{
  myfs_header header;
  blkdevsim->read(blockAddr(0), sizeof(header), (char *)&header);

  if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
      header.version != CURR_VERSION)
  {
    std::cout << "Did not find myfs instance on blkdev. Creating one..." << std::endl;
    format();
  }
}

// Search for free block and return the idx of it.
auto MyFs::findFreeBlock() -> int
{
  for (int i = 1; i < TOTAL_BLOCKS; i++)
  {
    uint8_t type;
    blkdevsim->read(blockAddr(i), sizeof(type), (char *)&type);
    if (type == BLOCK_TYPE_UNUSED)
      return i;
  }

  throw std::runtime_error("Disk is full! No free blocks left.");
}

// Format entire disk.
void MyFs::format(void)
{
  myfs_header header;
  strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
  header.version = CURR_VERSION;
  blkdevsim->write(blockAddr(0), sizeof(header), (const char *)&header);

  uint8_t zero = 0;
  for (int i = 1; i < TOTAL_BLOCKS; i++)
    blkdevsim->write(blockAddr(i), sizeof(zero), (const char *)&zero);
}

// Checks if a file with that name exists.
auto MyFs::fileExists(const std::string &path_str) -> bool
{
  for (size_t i{1}; i < TOTAL_BLOCKS; i++)
  {
    InodeBlock inode{};
    blkdevsim->read(blockAddr(i), sizeof(InodeBlock), (char *)&inode);
    if (inode.type == BLOCK_TYPE_INODE && path_str == inode.name)
      return true;
  }

  return false;
}

// Create a new empty file. (everything zeroed out)
auto MyFs::create_file(const std::string &path_str, bool directory) -> void
{
  if (directory)
    throw std::runtime_error("folders aren't supported.");

  if (fileExists(path_str))
    throw std::runtime_error("That file name is already taken.");

  int blk = findFreeBlock();

  InodeBlock inode{};
  inode.type = BLOCK_TYPE_INODE;
  inode.size = 0;
  inode.num_blocks = 0;
  strncpy(inode.name, path_str.c_str(), FILE_NAME_MAX_LEN);
  inode.name[FILE_NAME_MAX_LEN] = '\0';
  blkdevsim->write(blockAddr(blk), sizeof(InodeBlock), (const char *)&inode);
}

// Get a files contents. (go over all the blocks and concat them)
auto MyFs::get_content(const std::string &path_str) -> std::string
{
  for (size_t i{1}; i < TOTAL_BLOCKS; i++)
  {
    InodeBlock inode{};
    blkdevsim->read(blockAddr(i), sizeof(InodeBlock), (char *)&inode);

    if (inode.type != BLOCK_TYPE_INODE || path_str != inode.name)
      continue;

    std::string result;
    result.reserve(inode.size);
    int remaining = inode.size;

    for (size_t j{0}; j < inode.num_blocks && remaining > 0; j++)
    {
      ContentBlock cb{};
      blkdevsim->read(blockAddr(inode.blocks[j]), sizeof(ContentBlock), (char *)&cb);
      int chunk = std::min(remaining, DATA_PER_BLOCK);
      result.append(cb.data, chunk);
      remaining -= chunk;
    }

    return result;
  }

  throw std::runtime_error("Couldn't find that file.");
}

// break the content into chunks and update inodes.
auto MyFs::set_content(const std::string &path_str, const std::string &content) -> void
{
  for (size_t i{1}; i < TOTAL_BLOCKS; i++)
  {
    InodeBlock inode{};
    blkdevsim->read(blockAddr(i), sizeof(InodeBlock), (char *)&inode);

    if (inode.type != BLOCK_TYPE_INODE || path_str != inode.name)
      continue;

    // Clean up old data blocks so they can be reused.
    uint8_t zero = 0;
    for (int j = 0; j < inode.num_blocks; j++)
      blkdevsim->write(blockAddr(inode.blocks[j]), sizeof(zero), (const char *)&zero);

    inode.num_blocks = 0;
    inode.size = 0;

    // Figure out how many blocks we need and start writing the new stuff.
    int total = (int)content.size();
    int needed = (total + DATA_PER_BLOCK - 1) / DATA_PER_BLOCK;

    if (needed > MAX_CONTENT_BLOCKS)
      throw std::runtime_error("File is too big for this system.");

    int offset = 0;
    for (size_t j{}; j < needed; j++)
    {
      int blk = findFreeBlock();
      ContentBlock cb{};
      cb.type = BLOCK_TYPE_CONTENT;
      int chunk = std::min(total - offset, DATA_PER_BLOCK);
      memcpy(cb.data, content.c_str() + offset, chunk);
      blkdevsim->write(blockAddr(blk), sizeof(ContentBlock), (const char *)&cb);
      inode.blocks[inode.num_blocks++] = blk;
      offset += chunk;
    }

    // Save the updated file info to the disk.
    inode.size = total;
    blkdevsim->write(blockAddr(i), sizeof(InodeBlock), (const char *)&inode);
    return;
  }

  throw std::runtime_error("File not found.");
}

// (ls command) scan the whole disk and make a list of every file (Inode) and return it.
auto MyFs::list_dir(const std::string &path_str) -> dir_list
{
  dir_list ans{};

  for (size_t i{1}; i < TOTAL_BLOCKS; i++)
  {
    InodeBlock inode{};
    blkdevsim->read(blockAddr(i), sizeof(InodeBlock), (char *)&inode);

    if (inode.type == BLOCK_TYPE_INODE)
      ans.push_back({inode.name, false, inode.size});
  }

  return ans;
}
