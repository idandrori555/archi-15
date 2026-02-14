#include "myfs.h"
#include <iostream>
#include <math.h>
#include <ostream>
#include <sstream>
#include <string.h>

const char *MyFs::MYFS_MAGIC = "MYFS";

MyFs::MyFs(BlockDeviceSimulator *blkdevsim_) : blkdevsim(blkdevsim_)
{
  struct myfs_header header;
  blkdevsim->read(0, sizeof(header), (char *)&header);

  if (strncmp(header.magic, MYFS_MAGIC, sizeof(header.magic)) != 0 ||
      (header.version != CURR_VERSION))
  {
    std::cout << "Did not find myfs instance on blkdev" << std::endl;
    std::cout << "Creating..." << std::endl;
    format();
    std::cout << "Finished!" << std::endl;
  }
}

void MyFs::format()
{
  // put the header in place
  struct myfs_header header;
  strncpy(header.magic, MYFS_MAGIC, sizeof(header.magic));
  header.version = CURR_VERSION;
  blkdevsim->write(0, sizeof(header), (const char *)&header);

  // TODO: put your format code here
}

void MyFs::create_file(const std::string &path_str, bool directory)
{
  if (directory)
    throw std::runtime_error("directories not implemented"); // For now...

  return;
}

std::string MyFs::get_content(const std::string &path_str)
{
  throw std::runtime_error("not implemented");
  return "";
}

void MyFs::set_content(const std::string &path_str, const std::string &content)
{
  throw std::runtime_error("not implemented");
}

MyFs::dir_list MyFs::list_dir(const std::string &path_str)
{
  dir_list ans;
  throw std::runtime_error("not implemented");
  return ans;
}
