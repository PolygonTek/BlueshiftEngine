// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Platform/PlatformFile.h"
#include "File/FileSystem.h"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <jni.h>
#include <android/asset_manager.h>
#include <ftw.h>

BE_NAMESPACE_BEGIN

AAssetManager* PlatformAndroidFile::s_manager;
static Str _ExecutablePath;

_PlatformAndroidFile::_PlatformAndroidFile(FILE *fp) {
	assert(fp);
	m_fp = fp;
	m_asset = 0;
}

_PlatformAndroidFile::_PlatformAndroidFile(AAsset *asset) {
	assert(asset);
	m_fp = 0;
	m_asset = asset;
}

_PlatformAndroidFile::~_PlatformAndroidFile() {
	if (m_fp) {
		fclose(m_fp);
//		m_fp = 0;
	}
	else if (m_asset) {
		AAsset_close(m_asset);
//		m_asset = 0;
	}
}

int _PlatformAndroidFile::Tell() const {
	if (m_fp) {
		return ftell(m_fp);
	}
	else if (m_asset) {
		return  AAsset_getLength(m_asset) - AAsset_getRemainingLength(m_asset);
	}
	return -1;
}

int _PlatformAndroidFile::Size() const {
	if (m_fp) {
		struct stat fileInfo;
		fileInfo.st_size = -1;
		if (fstat(fileno(m_fp), &fileInfo) < 0)
			return -1;

		return fileInfo.st_size;
	}
	else if (m_asset) {
		return AAsset_getLength(m_asset);
	}
	return -1;
}

int _PlatformAndroidFile::Seek(long offset, Origin origin) {
	assert(offset >= 0);

	int whence;
	switch (origin) {
	case Start:
		whence = SEEK_SET;
		break;
	case End:
		whence = SEEK_END;
		break;
	case Current:
		whence = SEEK_CUR;
		break;
	default:
		assert(0);
		break;
	}

	if (m_fp) {
		return fseek(m_fp, offset, whence);
	}
	else if (m_asset) {
		return (AAsset_seek(m_asset, offset, whence) >= 0) ? 0 : -1;
	}
	return -1;
}

size_t _PlatformAndroidFile::Read(void *buffer, size_t bytesToRead) const {
	assert(bytesToRead > 0);
	if (m_fp) {
		size_t readBytes = fread(buffer, 1, bytesToRead, m_fp);
		return readBytes;
	} else if (m_asset) {
		off_t avail = AAsset_getRemainingLength(m_asset);
        if (avail == 0) {
            return 0;
        }
		if (bytesToRead > avail) {
            bytesToRead = avail;
        }
		jint readLength = AAsset_read(m_asset, buffer, bytesToRead);
		return (size_t)readLength;
	}
	return true;
}

bool _PlatformAndroidFile::Write(const void *buffer, size_t bytesToWrite) {
	byte *ptr = (byte *)buffer;
	size_t writeSize = bytesToWrite;
	bool failedOnce = false;

	while (writeSize) {
		size_t written;
		if (m_fp) {
			written = fwrite(ptr, 1, writeSize, m_fp);
		}
		else
			written = 0;

		if (written == 0) {
			BE_WARNLOG(L"_PlatformAndroidFile::Write: 0 bytes written");
			return false;
		}

		writeSize -= written;
		ptr += written;
	}

	if (m_fp)
		fflush(m_fp);

	return true;
}

////-------------------------------------------------------------------------------------------
//
Str _PlatformAndroidFile::NormalizeFilename(const char *filename) {
	Str normalizedFilename(basePath);
	normalizedFilename.AppendPath(filename);

	normalizedFilename.CleanPath('/');
	normalizedFilename.BackSlashesToSlashes();

	return normalizedFilename;
}

//Str _PlatformAndroidFile::NormalizeDirectoryName(const char *dirname) {
//	Str normalizedDirname(basePath);
//	normalizedDirname.AppendPath(dirname);
//	normalizedDirname.CleanPath('/');
//	//normalizedDirname.BackSlashesToSlashes();
//
//	int length = normalizedDirname.Length();
//	if (length > 0) {
//		if (normalizedDirname[length - 1] != '/') {
//			normalizedDirname.Append('/');
//		}
//	}
//
//	return normalizedDirname;
//}
//
//_PlatformAndroidFile *_PlatformAndroidFile::OpenFileRead(const char *filename) {
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	FILE *fp = fopen(normalizedFilename, "rb");
//	if (!fp) {
//		return NULL;
//	}
//
//	return new _PlatformAndroidFile(fp);
//}
//
//_PlatformAndroidFile *_PlatformAndroidFile::OpenFileWrite(const char *filename) {
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	FILE *fp = fopen(normalizedFilename, "wb");
//	if (!fp) {
//		return NULL;
//	}
//
//	return new _PlatformAndroidFile(fp);
//}
//
//_PlatformAndroidFile *_PlatformAndroidFile::OpenFileAppend(const char *filename) {
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	FILE *fp = fopen(normalizedFilename, "ab");
//	if (!fp) {
//		return NULL;
//	}
//
//	return new _PlatformAndroidFile(fp);
//}
//
//bool _PlatformAndroidFile::FileExists(const char *filename) {
//	struct stat fileInfo;
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	if (stat(normalizedFilename, &fileInfo) == 0 && S_ISREG(fileInfo.st_mode)) {
//		return true;
//	}
//	return false;
//}
//
//size_t _PlatformAndroidFile::FileSize(const char *filename) {
//	struct stat fileInfo;
//	fileInfo.st_size = -1;
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	stat(normalizedFilename, &fileInfo);
//
//	// make sure to return -1 for directories
//	if (S_ISDIR(fileInfo.st_mode)) {
//		fileInfo.st_size = -1;
//	}
//
//	return fileInfo.st_size;
//}
//
//bool _PlatformAndroidFile::IsFileWritable(const char *filename) {
//	struct stat fileInfo;
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	if (stat(normalizedFilename, &fileInfo) == -1) {
//		return true;
//	}
//	return (fileInfo.st_mode & S_IWUSR) != 0;
//}
//
//bool _PlatformAndroidFile::IsReadOnly(const char *filename) {
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	if (access(normalizedFilename, F_OK) == -1) {
//		return false; // file doesn't exist
//	}
//	if (access(normalizedFilename, W_OK) == -1) {
//		return errno == EACCES;
//	}
//	return false;
//}
//
bool _PlatformAndroidFile::SetReadOnly(const char *filename, bool readOnly) {
//	struct stat fileInfo;
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	if (stat(normalizedFilename, &fileInfo) == -1) {
//		return false;
//	}
//
//	if (readOnly) {
//		fileInfo.st_mode &= ~S_IWUSR;
//	}
//	else {
//		fileInfo.st_mode |= S_IWUSR;
//	}
//	return chmod(normalizedFilename, fileInfo.st_mode);
	assert(0);; return 0;
}
//
//bool _PlatformAndroidFile::RemoveFile(const char *filename) {
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	if (remove(normalizedFilename)) {
//		BLib::Log(NormalLog, L"failed to remove file '%hs'\n", normalizedFilename.c_str());
//		return false;
//	}
//
//	return true;
//}
//
//bool _PlatformAndroidFile::MoveFile(const char *srcFilename, const char *dstFilename) {
//	Str normalizedSrcFilename = PlatformFile::NormalizeFilename(srcFilename);
//	Str normalizedDstFilename = PlatformFile::NormalizeFilename(dstFilename);
//	return rename(normalizedSrcFilename, normalizedDstFilename) != 0 ? true : false;
//}
//
DateTime _PlatformAndroidFile::GetTimeStamp(const char *filename) {
	static const DateTime epoch(1970, 1, 1);
//	struct stat fileInfo;
//
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	if (stat(normalizedFilename, &fileInfo) == -1) {
//		return DateTime::MinValue();
//	}
//
//	Timespan timeSinceEpoch = Timespan::FromSeconds(fileInfo.st_mtime);
//	return epoch + timeSinceEpoch;
	assert(0);; return epoch;
}
//
void _PlatformAndroidFile::SetTimeStamp(const char *filename, const DateTime &timeStamp) {
//	static const DateTime epoch(1970, 1, 1);
//	struct stat fileInfo;
//
//	Str normalizedFilename = PlatformFile::NormalizeFilename(filename);
//	if (stat(normalizedFilename, &fileInfo) == -1) {
//		return;
//	}
//
//	struct utimbuf times;
//	times.actime = fileInfo.st_atime;
//	times.modtime = (timeStamp - epoch).TotalSeconds();
//	utime(filename, &times);
	assert(0);;
}
//
bool _PlatformAndroidFile::DirectoryExists(const char *dirname) {
	struct stat fileInfo;
//	Str normalizedDirname = PlatformFile::NormalizeDirectoryName(dirname);
	Str normalizedDirname = PlatformFile::ConvertToAndroidPath(dirname, true);
		if (stat(normalizedDirname, &fileInfo) == 0 && S_ISDIR(fileInfo.st_mode)) {
		return true;
	}
	return false;
}

static int mkdir_recursive(const char *fpath) {
	if (fpath[0] == 0)
		return 0;
	Str parent(fpath);
	parent.StripFileName();
	mkdir_recursive(parent);
	return mkdir(fpath, S_IRWXU | S_IRWXG | S_IRWXO);
}

bool _PlatformAndroidFile::CreateDirectory(const char *dirname) {
	if (PlatformFile::DirectoryExists(dirname)) {
		return true;
	}
	mkdir_recursive(basePath);
//	return mkdir(PlatformFile::NormalizeDirectoryName(dirname), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
	return mkdir(PlatformFile::ConvertToAndroidPath(dirname, true), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
}


bool _PlatformAndroidFile::RemoveDirectory(const char *dirname) {
//	Str normalizedDirname = PlatformFile::NormalizeDirectoryName(dirname);
	Str normalizedDirname = PlatformFile::ConvertToAndroidPath(dirname, true);
	return rmdir(normalizedDirname) == 0;
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
	int rv = remove(fpath);
	if (rv) {
		perror(fpath);
	}

	return rv;
}

bool _PlatformAndroidFile::RemoveDirectoryTree(const char *dirname) {
//	Str normalizedDirname = PlatformFile::NormalizeDirectoryName(dirname);
	Str normalizedDirname = PlatformFile::ConvertToAndroidPath(dirname, true);
	return nftw(normalizedDirname, unlink_cb, 64, FTW_DEPTH | FTW_PHYS) == 0;
}
//
//const char *_PlatformAndroidFile::Cwd() {
//	static char cwd[MaxAbsolutePath];
//
//	getcwd(cwd, sizeof(cwd) - 1);
//	cwd[sizeof(cwd) - 1] = 0;
//	return cwd;
//}
//
//const char *_PlatformAndroidFile::ExecutablePath() {
//	return Cwd();
//}
//
static void ListFilesRecursive(const char *directory, const char *subdir, const char *nameFilter, bool includeSubDir, Array<FileInfo> &files) {
	FileInfo    fileInfo;
	char		path[MaxAbsolutePath];
	char		subpath[MaxAbsolutePath];
	char		filename[MaxAbsolutePath];

	if (subdir[0]) {
		Str::snPrintf(path, sizeof(path), "%s/%s", directory, subdir);
	}
	else {
		Str::snPrintf(path, sizeof(path), "%s", directory);
	}

	DIR *dp = opendir(PlatformFile::ConvertToAndroidPath(path, true));
	if (!dp) {
		return;
	}

	while (dirent *dent = readdir(dp)) {
		if (!Str::Cmp(dent->d_name, ".") || !Str::Cmp(dent->d_name, "..")) {
			continue;
		}

		if (dent->d_type & DT_DIR) {
			if (subdir[0]) {
				Str::snPrintf(subpath, sizeof(subpath), "%s/%s", subdir, dent->d_name);
			}
			else {
				Str::snPrintf(subpath, sizeof(subpath), "%s", dent->d_name);
			}

			if (includeSubDir) {
				fileInfo.isSubDir = true;
				fileInfo.relativePath = subpath;
				files.Append(fileInfo);
			}

			ListFilesRecursive(directory, subpath, nameFilter, includeSubDir, files);
		}
		else if (Str::Filter(nameFilter, dent->d_name, false))	{
			if (subdir[0]) {
				Str::snPrintf(filename, sizeof(filename), "%s/%s", subdir, dent->d_name);
			}
			else {
				Str::snPrintf(filename, sizeof(filename), "%s", dent->d_name);
			}

			fileInfo.isSubDir = false;
			fileInfo.relativePath = filename;
			files.Append(fileInfo);
		}
	}

	closedir(dp);
	assert(0);;
}

int _PlatformAndroidFile::ListFiles(const char *directory, const char *nameFilter, bool recursive, bool includeSubDir, Array<FileInfo> &files) {
	if (!nameFilter) {
		nameFilter = "*";
	}

	files.Clear();

	Str normalizedDirectory = PlatformFile::NormalizeFilename(directory);

	if (recursive) {
		ListFilesRecursive(normalizedDirectory, "", nameFilter, includeSubDir, files);
	}
	else {
		AAssetDir* _dp = AAssetManager_openDir(PlatformFile::GetManager(), normalizedDirectory);
		if (_dp) {
			FileInfo fileInfo;
			while (const char* d_name = AAssetDir_getNextFileName(_dp)) {
				if (!Str::Filter(nameFilter, d_name)) {
					continue;
				}

				fileInfo.isSubDir = false;
				fileInfo.relativePath = d_name;

				files.Append(fileInfo);
			}

			AAssetDir_close(_dp);
		}
		if (!files.Count()) {
			DIR *dp = opendir(PlatformFile::ConvertToAndroidPath(normalizedDirectory, true));
			if (!dp) {
				return 0;
			}

			FileInfo fileInfo;
			while (dirent *dent = readdir(dp)) {
				if (!Str::Filter(nameFilter, dent->d_name)) {
					continue;
				}

				if (includeSubDir) {
					fileInfo.isSubDir = (dent->d_type & DT_DIR) ? true : false;
				}
				else {
					if (dent->d_type & DT_DIR) {
						continue;
					}
				}

				fileInfo.relativePath = dent->d_name;

				files.Append(fileInfo);
			}

			closedir(dp);
		}
	}

	return files.Count();
}


//////////////////////////////////////////

//PlatformAndroidFile::PlatformAndroidFile(NvFile *fp) : _PlatformAndroidFile(fp) {
//}

PlatformAndroidFile::~PlatformAndroidFile() {
}

Str PlatformAndroidFile::NormalizeFilename(const char *filename) {
    Str normalizedFilename;
    if (FileSystem::IsAbsolutePath(filename)) {
        normalizedFilename = filename;
    } else {
        normalizedFilename = basePath;
        normalizedFilename.AppendPath(filename);
    }
    normalizedFilename.CleanPath(PATHSEPERATOR_CHAR);
    
    return normalizedFilename;
}

Str PlatformAndroidFile::NormalizeDirectoryName(const char *dirname) {
    Str normalizedDirname;
    if (FileSystem::IsAbsolutePath(dirname)) {
        normalizedDirname = dirname;
    } else {
        normalizedDirname = basePath;
        normalizedDirname.AppendPath(dirname);
    }
    normalizedDirname.CleanPath(PATHSEPERATOR_CHAR);

    int length = normalizedDirname.Length();
    if (length > 0) {
        if (normalizedDirname[length - 1] != '/') {
            normalizedDirname.Append('/');
        }
    }
    
    return normalizedDirname;
}

PlatformAndroidFile *PlatformAndroidFile::OpenFileRead(const char *filename) {

	Str normalizedFilename = NormalizeFilename(filename);
	Str path = ConvertToAndroidPath(normalizedFilename, false);
	AAsset *asset = AAssetManager_open(s_manager, path, AASSET_MODE_UNKNOWN);
	if (asset)
		return new PlatformAndroidFile(asset);

	path = ConvertToAndroidPath(normalizedFilename, true);
	FILE *fp = fopen(path, "rb");
	if (fp)
		return new PlatformAndroidFile(fp);
	return 0;
}

PlatformAndroidFile *PlatformAndroidFile::OpenFileWrite(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);

	FILE *fp = fopen(ConvertToAndroidPath(normalizedFilename, true), "wb");

	if (!fp) {
		return NULL;
	}
    
    return new PlatformAndroidFile(fp);
}

PlatformAndroidFile *PlatformAndroidFile::OpenFileAppend(const char *filename) {
    Str normalizedFilename = NormalizeFilename(filename);
	FILE *fp = fopen(ConvertToAndroidPath(normalizedFilename, true), "ab");
    if (!fp) {
        return NULL;
    }
    
    return new PlatformAndroidFile(fp);
}

bool PlatformAndroidFile::FileExists(const char *filename) {
    //struct stat fileInfo;
    //Str normalizedFilename = NormalizeFilename(filename);
    //if (stat(ConvertToIOSPath(normalizedFilename, false), &fileInfo) != 0) {
    //    if (stat(ConvertToIOSPath(normalizedFilename, true), &fileInfo) != 0) {
    //        return false;
    //    }
    //}
    //return S_ISREG(fileInfo.st_mode);
	assert(0);; return 0;
}

size_t PlatformAndroidFile::FileSize(const char *filename) {
    //struct stat fileInfo;
    //fileInfo.st_size = -1;
    //Str normalizedFilename = NormalizeFilename(filename);
    //if (stat(ConvertToIOSPath(normalizedFilename, false), &fileInfo) != 0) {
    //    if (stat(ConvertToIOSPath(normalizedFilename, true), &fileInfo) != 0) {
    //        return false;
    //    }
    //}
    //
    //// make sure to return -1 for directories
    //if (S_ISDIR(fileInfo.st_mode)) {
    //    fileInfo.st_size = -1;
    //}
    //
    //return fileInfo.st_size;
	assert(0);; return 0;
}

bool PlatformAndroidFile::IsFileWritable(const char *filename) {
    //struct stat fileInfo;
    //Str normalizedFilename = NormalizeFilename(filename);
    //if (stat(ConvertToIOSPath(normalizedFilename, false), &fileInfo) != 0) {
    //    if (stat(ConvertToIOSPath(normalizedFilename, true), &fileInfo) != 0) {
    //        return false;
    //    }
    //}
    //return (fileInfo.st_mode & S_IWUSR) != 0;
	assert(0);; return 0;
}

bool PlatformAndroidFile::IsReadOnly(const char *filename) {
    //Str normalizedFilename = NormalizeFilename(filename);
    //Str filePath = ConvertToIOSPath(normalizedFilename, false);
    //if (access(filePath, F_OK) == -1) {
    //    if (access(ConvertToIOSPath(normalizedFilename, true), F_OK) == -1) {
    //        return false; // file doesn't exist
    //    }
    //}
    //
    //if (access(filePath, W_OK) == -1) {
    //    return errno == EACCES;
    //}
    //return false;
	assert(0);; return 0;
}

bool PlatformAndroidFile::RemoveFile(const char *filename) {
    //Str normalizedFilename = NormalizeFilename(filename);
    //return unlink(ConvertToIOSPath(normalizedFilename, true));
	assert(0);; return 0;
}

bool PlatformAndroidFile::MoveFile(const char *srcFilename, const char *dstFilename) {
    //Str normalizedSrcFilename = ConvertToIOSPath(NormalizeFilename(srcFilename), false);
    //Str normalizedDstFilename = ConvertToIOSPath(NormalizeFilename(dstFilename), true);
    //return rename(normalizedSrcFilename, normalizedDstFilename) != -1;
	assert(0);; return 0;
}

const char *PlatformAndroidFile::ExecutablePath() {
    //static NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    //static const char *cstr = (const char *)[bundlePath cStringUsingEncoding:NSUTF8StringEncoding];
    //return cstr;
	return _ExecutablePath.c_str();
}

void	PlatformAndroidFile::SetExecutablePath(const Str &path)
{
	_ExecutablePath = path;
}

Str PlatformAndroidFile::ConvertToAndroidPath(const Str &filename, bool forWrite) {
	//Str result;

	if (forWrite) {
		return  Str(ExecutablePath()) + "/" + filename;
	}
	else {
		return filename;

	}
}

BE_NAMESPACE_END
