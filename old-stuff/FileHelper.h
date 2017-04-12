/*
 * FileOps.h
 *
 *  Created on: Jul 25, 2016
 *      Author: tsokalo
 */

#ifndef FILEOPS_H_
#define FILEOPS_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <time.h>
#include <string.h>
#include <assert.h>

struct LogInfo
{
  LogInfo ()
  {

  }
  LogInfo (uint16_t nodeId, std::string name)
  {
    this->nodeId = nodeId;
    this->name = name;
    p1 = 0;
  }
  LogInfo (uint16_t nodeId, std::string name, double p1)
  {
    this->nodeId = nodeId;
    this->name = name;
    this->p1 = p1;
  }
  uint16_t nodeId;
  std::string name;
  double p1;
};

class FileHelper
{
public:
  FileHelper (LogInfo info, std::string resDir) :
    m_resDir (resDir)
  {
    OpenFile (info);
  }
  ~FileHelper ()
  {
    Close ();
  }

  void
  OpenFile (LogInfo logInfo)
  {
    std::stringstream ss;
    ss << logInfo.name << "_" << logInfo.nodeId << "_" << logInfo.p1 << ".txt";

    std::string fileName = m_resDir + ss.str ();
    m_file.open (fileName.c_str (), std::fstream::out | std::fstream::trunc);
    assert(m_file.is_open());
  }
  void
  Write (uint16_t eventId, double v)
  {
    if (m_file.is_open ()) m_file << eventId << "\t" << v << std::endl;
  }
  void
  Write (uint16_t eventId, double v1, double v2)
  {
    if (m_file.is_open ()) m_file << eventId << "\t" << v1 << "\t" << v2 << std::endl;
  }
  void
  Write (uint16_t eventId, double v1, double v2, double v3)
  {
    if (m_file.is_open ()) m_file << eventId << "\t" << v1 << "\t" << v2 << "\t" << v3 << std::endl;
  }
  void
  Write (uint16_t eventId, double v1, double v2, double v3, double v4)
  {
    if (m_file.is_open ()) m_file << eventId << "\t" << v1 << "\t" << v2 << "\t" << v3 << "\t" << v4 << std::endl;
  }

private:

  void
  Close ()
  {
    m_file.close ();
  }

  std::ofstream m_file;
  std::string m_resDir;
  bool m_file3DOpened;
  bool m_file2DOpened;
  std::ofstream m_file3D;
  std::ofstream m_file2D;
};

#endif /* FILEOPS_H_ */
