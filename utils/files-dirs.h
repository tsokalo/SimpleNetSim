/*
 * os-dirs.h
 *
 *  Created on: 31.01.2018
 *      Author: tsokalo
 */

#ifndef UTILS_FILES_DIRS_H_
#define UTILS_FILES_DIRS_H_

#include <string.h>
#include <sstream>
#include <vector>
#include <iostream>
#include <iomanip>


#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <stdexcept>
#include <dirent.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <iterator>
#include <fstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <vector>

#include <fcntl.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "log.h"

typedef std::vector<std::string> FileList;
void
GetDirListing(FileList& result, const std::string& dirpath);
std::string
rtrim(std::string str);
std::string
trim(std::string str);

bool
ExecuteCommand(std::string command);

int
CreateDirectory(std::string path);
bool
IsDirectory(std::string path);
bool
RemoveDirectory(std::string path);
bool
CleanDirectory(std::string folderPath, std::string namePart);
bool
CopyDirectory(std::string srcDir, std::string dstDir);
/*
 * performs deep search
 */
bool
FindFolder(std::string searchPath, std::string folderName, std::string &matchFullPath);
bool
GetDirectorySize(std::string folderPath, double &dirSize);
/*
 * no deep search. Looking only in the provided directory for the file with the specified name part
 */
std::vector<std::string>
FindFile(std::string searchPath, std::string filePartName);
bool
CopyFile(std::string srcDir, std::string dstDir, std::string fileName);
void
LoadFiles(std::vector<std::string> &files, std::string folderPath);
bool
IsFileCreated(std::string filePath);
long
GetFileSize(std::string filename);

int
ConvertFileToStr(std::string fileName, std::string &str);
int
ConvertStrToFile(std::string fileName, std::string str);

bool
DeleteFiles(std::string path);
bool
DeleteFile(std::string path);


void GetDirListing(FileList& result, const std::string& dirpath) {
	DIR* dir = opendir(dirpath.c_str());
	if (dir) {
		struct dirent* entry;
		while ((entry = readdir(dir))) {
			struct stat entryinfo;
			std::string entryname = entry->d_name;
			std::string entrypath = dirpath + "/" + entryname;
			if (!stat(entrypath.c_str(), &entryinfo)) result.push_back(entrypath);
		}
		closedir(dir);
	}
}


std::string rtrim(std::string str) {
	int32_t pos = 0;
	pos = str.rfind("/");
	pos = (pos < 0) ? 0 : pos;
	int32_t end = strlen(str.c_str());
	str = str.substr(pos, end);
	return str;
}

std::string trim(std::string str) {
	int32_t pos = 0;
	pos = str.rfind("/");
	pos = (pos < 0) ? 0 : pos;
	//    int32_t end = strlen (str.c_str ());
	str = str.substr(0, pos);
	return str;
}

bool ExecuteCommand(std::string command) {
	SIM_LOG(FILEOPER_LOG, command);
	return (std::system(command.c_str()) < 0) ? false : true;
}

typedef struct stat Stat;
int CreateDirectory(std::string path) {
	//mode_t mode = 0x0666;
	Stat st;
	int32_t status = 0;

	if (stat(path.c_str(), &st) != 0) {
		/* Directory does not exist. EEXIST for race condition */
		if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST) status = -1;//, mode
	}
	else if (!S_ISDIR(st.st_mode)) {
		errno = ENOTDIR;
		status = -1;
	}

	return status;
}

bool IsDirectory(std::string path) {
	for (uint32_t i = 0; i < path.size(); i++) {
		if (path.at(i) == '/') return true;
	}
	return false;
}

bool RemoveDirectory(std::string folderPath) {
	SIM_LOG(1, "Deleting directory: " << folderPath);
	FileList dirtree;
	GetDirListing(dirtree, folderPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR (st_buf.st_mode)) {
			RemoveDirectory(fullPath);
		}
		else {
			std::remove(fullPath.c_str());
		}
		rmdir(fullPath.c_str());
	}
	return true;
}
bool CleanDirectory(std::string folderPath, std::string namePart) {
	SIM_LOG(FILEOPER_LOG, "Cleaning directory: " << folderPath);

	FileList dirtree;
	GetDirListing(dirtree, folderPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR (st_buf.st_mode)) {
			CleanDirectory(fullPath, namePart);
		}
		else {
			if (str.find(namePart, 0) != std::string::npos) std::remove(fullPath.c_str());
		}
	}
	return true;
}
bool CopyDirectory(std::string srcDir, std::string dstDir) {
	SIM_LOG(FILEOPER_LOG, "Copying directory: " << srcDir << " to " << dstDir);
	std::string newDirPath = dstDir + rtrim(srcDir);
	if (mkdir(newDirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
		struct stat st_buf;
		stat(newDirPath.c_str(), &st_buf);
		if (!S_ISDIR (st_buf.st_mode)) {
			SIM_LOG(FILEOPER_LOG, "Failed to create the dir: " << newDirPath);
			return false;
		}
	}

	FileList dirtree;
	GetDirListing(dirtree, newDirPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR (st_buf.st_mode)) {
			if (!CopyDirectory(fullPath, newDirPath)) return false;
		}
		else {
			if (!CopyFile(srcDir, dstDir, rtrim(fullPath))) return false;
		}
	}
	return true;
}


bool FindFolder(std::string searchPath, std::string folderName, std::string &matchFullPath) {
	SIM_LOG(FILEOPER_LOG, "Searching in directory: " << searchPath << " for " << folderName);

	FileList dirtree;
	GetDirListing(dirtree, searchPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR (st_buf.st_mode)) {
			if (str == folderName) {
				matchFullPath = fullPath;
				return true;
			}

			if (FindFolder(fullPath, folderName, matchFullPath)) return true;
		}
		else {
			continue;
		}
	}
	return false;
}

bool GetDirectorySize(std::string folderPath, double &dirSize) {
	SIM_LOG(FILEOPER_LOG, "Getting size for directory: " << folderPath);

	FileList dirtree;
	GetDirListing(dirtree, folderPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR (st_buf.st_mode)) {
			if (!GetDirectorySize(fullPath, dirSize)) return false;
		}
		else {
			dirSize += GetFileSize(fullPath);
		}
	}
	return true;
}

std::vector<std::string> FindFile(std::string searchPath, std::string filePartName) {
	SIM_LOG(FILEOPER_LOG, "Searching in directory: " << searchPath << " for file with " << filePartName << " in its name");
	std::vector<std::string> matchFullPath;
	FileList dirtree;
	GetDirListing(dirtree, searchPath);
	int32_t numofpaths = dirtree.size();

	for (int32_t i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int32_t pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		struct stat st_buf;
		stat(fullPath.c_str(), &st_buf);
		if (S_ISDIR (st_buf.st_mode)) {
			continue;
		}
		else {
			std::size_t found = str.find(filePartName);
			if (found != std::string::npos) {
				matchFullPath.push_back(fullPath);
				SIM_LOG(FILEOPER_LOG, "Found file: " << fullPath);
			}
		}
	}
	return matchFullPath;
}
bool CopyFile(std::string srcDir, std::string dstDir, std::string fileName) {
	SIM_LOG(FILEOPER_LOG, "Copy file: " << fileName << " from " << srcDir << " to " << dstDir);
	//    std::string outputPath = srcDir + fileName;
	//    std::string inputPath = dstDir + fileName;

	std::string comm = "cp " + srcDir + fileName + " " + dstDir;

	return ExecuteCommand(comm.c_str());

	//    std::ifstream ifs (outputPath.c_str (), std::ios::in | std::ios::binary);
	//    std::ofstream ofs (inputPath.c_str (), std::ios::out | std::ios::trunc | std::ios::binary);
	//    if (ifs.is_open () && ofs.is_open ())
	//    {
	//        if (ifs.is_open ()) ifs.close ();
	//        if (ofs.is_open ()) ofs.close ();
	//        ofs << ifs.rdbuf ();
	//    }
	//    else
	//    {
	//        if (ifs.is_open ()) ifs.close ();
	//        if (ofs.is_open ()) ofs.close ();
	//        return false;
	//    }

	//    return true;
}

void LoadFiles(std::vector<std::string> &files, std::string folderPath) {
	FileList dirtree;
	GetDirListing(dirtree, folderPath);
	int numofpaths = dirtree.size();

	for (int i = 0; i < numofpaths; i++) {
		std::string str(dirtree[i]);
		std::string fullPath = str;

		int pos = 0;
		while (pos != -1) {
			pos = str.find("/");
			str = str.substr(pos + 1);
		}
		if (str == "" || str == "." || str == "..") {
			continue;
		}
		files.push_back(fullPath);
	}
}
bool IsFileCreated(std::string filePath) {
	struct stat buffer;
	return (stat(filePath.c_str(), &buffer) == 0);
}

long GetFileSize(std::string filename) {
	struct stat stat_buf;
	int32_t rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

int ConvertFileToStr(std::string fileName, std::string &contents) {
	SIM_LOG(FILEOPER_LOG, "Trying to open the file: " << fileName);
	std::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) return -1;
	std::stringstream strStream;
	strStream << file.rdbuf();
	contents = strStream.str();
	SIM_LOG(FILEOPER_LOG, "Size of the converted to string file: " << contents.size());
	file.close();
	return 0;
}
int ConvertStrToFile(std::string fileName, std::string str) {
	std::ofstream file(fileName.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
	if (!file.is_open()) return -1;
	file << str;
	file.close();
	return 0;
}

bool DeleteFiles(std::string path) {
	std::string cmd = "rm -R " + path + "*";
	if (std::system(cmd.c_str()) < 0) return false;
	return true;
}
bool DeleteFile(std::string path) {
	SIM_LOG(FILEOPER_LOG, "Deleting file: " << path);
	std::string cmd = "rm " + path;
	if (std::system(cmd.c_str()) < 0) return false;
	return true;
}



#endif /* UTILS_FILES_DIRS_H_ */
