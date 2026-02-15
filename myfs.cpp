#include "myfs.h"
#include <iostream>
#include <stdexcept>
#include <string.h>

/**
 * Connects the block device and checks FS.
 */
MyFs::MyFs(BlockDeviceSimulator *blkdevsim_) : blkdevsim(blkdevsim_)
{
  struct myfs_header header;
  blkdevsim->read(0, sizeof(header), (char *)&header);

  // CMP magic and version
  if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
      (header.version != CURR_VERSION))
  {
    std::cout << "Did not find myfs instance on blkdev. Creating..." << std::endl;
    format();
  }
}

/**
 * Resets the entire disk structure.
 */
void MyFs::format(void)
{
  // Write FS Header
  struct myfs_header header;
  strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
  header.version = CURR_VERSION;
  blkdevsim->write(0, sizeof(header), (const char *)&header);

  // Init Metadata table (empyty)
  FileEntry empty_entry{};
  memset(&empty_entry, 0, sizeof(empty_entry));
  empty_entry.is_used = false;

  for (size_t i{}; i < MAX_FILES; i++)
  {
    blkdevsim->write(METADATA_OFFSET + (i * sizeof(FileEntry)), sizeof(FileEntry), (const char *)&empty_entry);
  }
}

/**
 * Add a new file to the metadata table.
 */
auto MyFs::create_file(const std::string &path_str, bool directory) -> void
{
  if (directory)
  {
    throw std::runtime_error("Directories not implemented");
    return;
  }

  // Look for free slot in metadata table to insert our new file into.
  for (size_t i{}; i < MAX_FILES; i++)
  {
    FileEntry entry{};
    blkdevsim->read(METADATA_OFFSET + (i * sizeof(FileEntry)), sizeof(FileEntry), (char *)&entry);

    if (!entry.is_used)
    {
      memset(&entry, 0, sizeof(entry));
      strncpy(entry.name, path_str.c_str(), FILE_NAME_MAX_LENGTH); // Limit name
      entry.is_used = true;
      entry.size = 0;

      // start after the i'th file
      entry.start_address = DATA_START_OFFSET + (i * FILE_MAX_SIZE);

      // Update disk
      blkdevsim->write(METADATA_OFFSET + (i * sizeof(FileEntry)), sizeof(FileEntry), (const char *)&entry);
      return;
    }
  }
  throw std::runtime_error("File system full: Maximum file count reached");
}

/**
 * Reads the file data from the disk.
 */
auto MyFs::get_content(const std::string &path_str) -> std::string
{
  for (size_t i{}; i < MAX_FILES; i++)
  {
    FileEntry entry{};
    blkdevsim->read(METADATA_OFFSET + (i * sizeof(FileEntry)), sizeof(FileEntry), (char *)&entry);

    if (entry.is_used && path_str == entry.name)
    {
      if (0 == entry.size)
        return "";

      // Buffer for reading data
      std::vector<char> buffer(entry.size);
      blkdevsim->read(entry.start_address, entry.size, buffer.data());
      return std::string(buffer.begin(), buffer.end());
    }
  }
  throw std::runtime_error("File not found");
}

/**
 * Write a file content and updates its size. (metadata)
 */
auto MyFs::set_content(const std::string &path_str, const std::string &content) -> void
{
  if (FILE_MAX_SIZE < content.length())
  {
    throw std::runtime_error("File content too large for the allocated block");
  }

  for (size_t i{}; i < MAX_FILES; i++)
  {
    FileEntry entry{};
    blkdevsim->read(METADATA_OFFSET + (i * sizeof(FileEntry)), sizeof(FileEntry), (char *)&entry);

    if (entry.is_used && path_str == entry.name)
    {
      entry.size = content.length();

      // Write raw data to the file
      blkdevsim->write(entry.start_address, entry.size, content.c_str());

      // Update metadata (size changed so we neeed to update it )
      blkdevsim->write(METADATA_OFFSET + (i * sizeof(FileEntry)), sizeof(FileEntry), (const char *)&entry);
      return;
    }
  }
  throw std::runtime_error("File not found");
}

/**
 * lists all used files in metadata table.
 */
auto MyFs::list_dir(const std::string &path_str) -> dir_list
{
  dir_list ans{};
  for (size_t i{}; i < MAX_FILES; i++)
  {
    FileEntry entry{};
    blkdevsim->read(METADATA_OFFSET + (i * sizeof(FileEntry)), sizeof(FileEntry), (char *)&entry);

    if (entry.is_used)
    {
      dir_list_entry de;
      de.name = entry.name;
      de.is_dir = false;
      de.file_size = entry.size;
      ans.push_back(de);
    }
  }
  return ans;
}
