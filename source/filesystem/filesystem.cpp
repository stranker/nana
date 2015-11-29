/*
 *	A ISO C++ FileSystem Implementation
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/filesystem/filesystem.cpp
 *	@description:
 *		provide some interface for file managment
 */

#include <nana/filesystem/filesystem.hpp>
#include <vector>
#if defined(NANA_WINDOWS)
    #include <windows.h>

    #if defined(NANA_MINGW)
        #ifndef _WIN32_IE
            #define _WIN32_IE 0x0500
        #endif
    #endif

	#include <shlobj.h>
	#include <nana/datetime.hpp>
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
	#include <nana/charset.hpp>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <dirent.h>
	#include <cstdio>
	#include <cstring>
	#include <errno.h>
	#include <unistd.h>
	#include <stdlib.h>
#endif

namespace nana {
	namespace experimental
	{
		namespace filesystem
		{
			//Because of No wide character version of POSIX
#if defined(NANA_LINUX) || defined(NANA_MACOS)
			const char* splstr = "/";
#else
			const wchar_t* splstr = L"/\\";
#endif

			//class path
			path::path() {}

			int path::compare(const path& p) const
			{
				return pathstr_.compare(p.pathstr_);
			}

			bool path::empty() const
			{
#if defined(NANA_WINDOWS)
				return (::GetFileAttributes(pathstr_.c_str()) == INVALID_FILE_ATTRIBUTES);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				struct stat sta;
				return (::stat(pathstr_.c_str(), &sta) == -1);
#endif
			}

			path path::extension() const
			{
#if defined(NANA_WINDOWS)
				auto pos = pathstr_.find_last_of(L"\\/.");
#else
				auto pos = pathstr_.find_last_of("\\/.");
#endif
				if ((pos == pathstr_.npos) || (pathstr_[pos] != '.'))
					return path();

				
				if (pos + 1 == pathstr_.size())
					return path();

				return path(pathstr_.substr(pos));
			}

			path path::parent_path() const
			{
				return{filesystem::parent_path(pathstr_)};
			}

			file_type path::what() const
			{
#if defined(NANA_WINDOWS)
				unsigned long attr = ::GetFileAttributes(pathstr_.c_str());
				if (INVALID_FILE_ATTRIBUTES == attr)
					return file_type::not_found; //??

				if (FILE_ATTRIBUTE_DIRECTORY & attr)
					return file_type::directory;

				return file_type::regular;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				struct stat sta;
				if (-1 == ::stat(pathstr_.c_str(), &sta))
					return file_type::not_found; //??

				if ((S_IFDIR & sta.st_mode) == S_IFDIR)
					return file_type::directory;

				if ((S_IFREG & sta.st_mode) == S_IFREG)
					return file_type::regular;

				return file_type::none;
#endif
			}

			path path::filename() const
			{
				auto pos = pathstr_.find_last_of(splstr);
				if (pos != pathstr_.npos)
				{
					if (pos + 1 == pathstr_.size())
					{
						value_type tmp[2] = {preferred_separator, 0};

						if (pathstr_.npos != pathstr_.find_last_not_of(splstr, pos))
							tmp[0] = '.';

						return{ tmp };
					}
					return{ pathstr_.substr(pos + 1) };
				}

				return{ pathstr_ };
			}

			const path::value_type* path::c_str() const
			{
				return native().c_str();
			}

			const path::string_type& path::native() const
			{
				return pathstr_;
			}
			
			path::operator string_type() const
			{
				return native();
			}

			void path::_m_assign(const std::string& source_utf8)
			{
#if defined(NANA_WINDOWS)
				pathstr_ = utf8_cast(source_utf8);
#else
				pathstr_ = source_utf8;
#endif
			}

			void path::_m_assign(const std::wstring& source)
			{
#if defined(NANA_WINDOWS)
				pathstr_ = source;
#else
				pathstr_ = utf8_cast(source);
#endif			
			}
			//end class path

			bool operator==(const path& lhs, const path& rhs)
			{
				return (lhs.compare(rhs) == 0);
			}

			bool operator!=(const path& lhs, const path& rhs)
			{
				return (lhs.native() != rhs.native());
			}

			bool operator<(const path& lhs, const path& rhs)
			{
				return (lhs.compare(rhs) < 0);
			}

			bool operator>(const path& lhs, const path& rhs)
			{
				return (rhs.compare(lhs) < 0);
			}

			namespace detail
			{
				//rm_dir_recursive
				//@brief: remove a directory, if it is not empty, recursively remove it's subfiles and sub directories
				template<typename CharT>
				bool rm_dir_recursive(const CharT* dir)
				{
					std::vector<directory_iterator::value_type> files;
					std::basic_string<CharT> path = dir;
					path += '\\';

					std::copy(directory_iterator(dir), directory_iterator(), std::back_inserter(files));

					for (auto & f : files)
					{
						auto subpath = path + f.path().filename().native();
						if (f.attr.directory)
							rm_dir_recursive(subpath.c_str());
						else
							rmfile(subpath.c_str());
					}

					return rmdir(dir, true);
				}

				bool mkdir_helper(const nana::string& dir, bool & if_exist)
				{
#if defined(NANA_WINDOWS)
					if (::CreateDirectory(dir.c_str(), 0))
					{
						if_exist = false;
						return true;
					}

					if_exist = (::GetLastError() == ERROR_ALREADY_EXISTS);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
					if (0 == ::mkdir(static_cast<std::string>(nana::charset(dir)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
					{
						if_exist = false;
						return true;
					}

					if_exist = (errno == EEXIST);
#endif
					return false;
				}

#if defined(NANA_WINDOWS)
				void filetime_to_c_tm(FILETIME& ft, struct tm& t)
				{
					FILETIME local_file_time;
					if (::FileTimeToLocalFileTime(&ft, &local_file_time))
					{
						SYSTEMTIME st;
						::FileTimeToSystemTime(&local_file_time, &st);
						t.tm_year = st.wYear - 1900;
						t.tm_mon = st.wMonth - 1;
						t.tm_mday = st.wDay;
						t.tm_wday = st.wDayOfWeek - 1;
						t.tm_yday = nana::date::day_in_year(st.wYear, st.wMonth, st.wDay);

						t.tm_hour = st.wHour;
						t.tm_min = st.wMinute;
						t.tm_sec = st.wSecond;
					}
				}
#endif
			}//end namespace detail

			bool file_attrib(const nana::string& file, attribute& attr)
			{
#if defined(NANA_WINDOWS)
				WIN32_FILE_ATTRIBUTE_DATA fad;
				if (::GetFileAttributesEx(file.c_str(), GetFileExInfoStandard, &fad))
				{
					LARGE_INTEGER li;
					li.u.LowPart = fad.nFileSizeLow;
					li.u.HighPart = fad.nFileSizeHigh;
					attr.size = li.QuadPart;
					attr.directory = (0 != (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
					detail::filetime_to_c_tm(fad.ftLastWriteTime, attr.modified);
					return true;
				}
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				struct stat fst;
				if (0 == ::stat(static_cast<std::string>(nana::charset(file)).c_str(), &fst))
				{
					attr.size = fst.st_size;
					attr.directory = (0 != (040000 & fst.st_mode));
					attr.modified = *(::localtime(&fst.st_ctime));
					return true;
				}
#endif
				return false;
			}

			uintmax_t file_size(const nana::string& file)
			{
#if defined(NANA_WINDOWS)
				//Some compilation environment may fail to link to GetFileSizeEx
				typedef BOOL(__stdcall *GetFileSizeEx_fptr_t)(HANDLE, PLARGE_INTEGER);
				GetFileSizeEx_fptr_t get_file_size_ex = reinterpret_cast<GetFileSizeEx_fptr_t>(::GetProcAddress(::GetModuleHandleA("Kernel32.DLL"), "GetFileSizeEx"));
				if (get_file_size_ex)
				{
					HANDLE handle = ::CreateFile(file.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					if (INVALID_HANDLE_VALUE != handle)
					{
						LARGE_INTEGER li;
						if (!get_file_size_ex(handle, &li))
							li.QuadPart = 0;

						::CloseHandle(handle);
						return li.QuadPart;
					}
				}
				return 0;
#elif defined(NANA_LINUX)
				FILE * stream = ::fopen(static_cast<std::string>(nana::charset(file)).c_str(), "rb");
				long long size = 0;
				if (stream)
				{
					fseeko64(stream, 0, SEEK_END);
					size = ftello64(stream);
					fclose(stream);
				}
				return size;
#elif defined(NANA_MACOS)
				FILE * stream = ::fopen(static_cast<std::string>(nana::charset(file)).c_str(), "rb");
				long long size = 0;
				if (stream)
				{
					fseeko(stream, 0, SEEK_END);
					size = ftello(stream);
					fclose(stream);
				}
				return size;				
#endif
			}

			bool modified_file_time(const nana::string& file, struct tm& t)
			{
#if defined(NANA_WINDOWS)
				WIN32_FILE_ATTRIBUTE_DATA attr;
				if (::GetFileAttributesEx(file.c_str(), GetFileExInfoStandard, &attr))
				{
					FILETIME local_file_time;
					if (::FileTimeToLocalFileTime(&attr.ftLastWriteTime, &local_file_time))
					{
						SYSTEMTIME st;
						::FileTimeToSystemTime(&local_file_time, &st);
						t.tm_year = st.wYear - 1900;
						t.tm_mon = st.wMonth - 1;
						t.tm_mday = st.wDay;
						t.tm_wday = st.wDayOfWeek - 1;
						t.tm_yday = nana::date::day_in_year(st.wYear, st.wMonth, st.wDay);

						t.tm_hour = st.wHour;
						t.tm_min = st.wMinute;
						t.tm_sec = st.wSecond;
						return true;
					}
				}
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				struct stat attr;
				if (0 == ::stat(static_cast<std::string>(nana::charset(file)).c_str(), &attr))
				{
					t = *(::localtime(&attr.st_ctime));
					return true;
				}
#endif
				return false;
			}

			bool create_directory(const nana::string& path, bool & if_exist)
			{
				if_exist = false;
				if (path.size() == 0) return false;

				nana::string root;
#if defined(NANA_WINDOWS)
				if (path.size() > 3 && path[1] == L':')
					root = path.substr(0, 3);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				if (path[0] == STR('/'))
					root = '/';
#endif
				bool mkstat = false;
				std::size_t beg = root.size();

				while (true)
				{
					beg = path.find_first_not_of(L"/\\", beg);
					if (beg == path.npos)
						break;

					std::size_t pos = path.find_first_of(L"/\\", beg + 1);
					if (pos != path.npos)
					{
						root += path.substr(beg, pos - beg);

						mkstat = detail::mkdir_helper(root, if_exist);
						if (mkstat == false && if_exist == false)
							return false;

#if defined(NANA_WINDOWS)
						root += L'\\';
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
						root += L'/';
#endif
					}
					else
					{
						if (beg + 1 < path.size())
						{
							root += path.substr(beg);
							mkstat = detail::mkdir_helper(root, if_exist);
						}
						break;
					}
					beg = pos + 1;
				}
				return mkstat;
			}

			bool rmfile(const nana::char_t* file)
			{
#if defined(NANA_WINDOWS)
				bool ret = false;
				if (file)
				{
					ret = (::DeleteFile(file) == TRUE);
					if (!ret)
						ret = (ERROR_FILE_NOT_FOUND == ::GetLastError());
				}

				return ret;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				if (std::remove(static_cast<std::string>(nana::charset(file)).c_str()))
					return (errno == ENOENT);
				return true;
#endif
			}

			bool rmdir(const char* dir_utf8, bool fails_if_not_empty)
			{
				bool ret = false;
				if (dir_utf8)
				{
#if defined(NANA_WINDOWS)
					auto dir = utf8_cast(dir_utf8);
					ret = (::RemoveDirectory(dir.c_str()) == TRUE);
					if (!fails_if_not_empty && (::GetLastError() == ERROR_DIR_NOT_EMPTY))
						ret = detail::rm_dir_recursive(dir.c_str());
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
					if (::rmdir(dir_utf8))
					{
						if (!fails_if_not_empty && (errno == EEXIST || errno == ENOTEMPTY))
							ret = detail::rm_dir_recursive(dir);
					}
					else
						ret = true;
#endif
				}
				return ret;
			}

			bool rmdir(const wchar_t* dir, bool fails_if_not_empty)
			{
				bool ret = false;
				if (dir)
				{
#if defined(NANA_WINDOWS)
					ret = (::RemoveDirectory(dir) == TRUE);
					if (!fails_if_not_empty && (::GetLastError() == ERROR_DIR_NOT_EMPTY))
						ret = detail::rm_dir_recursive(dir);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
					if (::rmdir(utf8_cast(dir).c_str()))
					{
						if (!fails_if_not_empty && (errno == EEXIST || errno == ENOTEMPTY))
							ret = detail::rm_dir_recursive(dir);
					}
					else
						ret = true;
#endif
				}
				return ret;
			}

			nana::string path_user()
			{
#if defined(NANA_WINDOWS)
				nana::char_t path[MAX_PATH];
				if (SUCCEEDED(SHGetFolderPath(0, CSIDL_PROFILE, 0, SHGFP_TYPE_CURRENT, path)))
					return path;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				const char * s = ::getenv("HOME");
				if (s)
					return nana::charset(std::string(s, std::strlen(s)), nana::unicode::utf8);
#endif
				return nana::string();
			}

			path current_path()
			{
#if defined(NANA_WINDOWS)
				nana::char_t buf[MAX_PATH];
				DWORD len = ::GetCurrentDirectory(MAX_PATH, buf);
				if (len)
				{
					if (len > MAX_PATH)
					{
						nana::char_t * p = new nana::char_t[len + 1];
						::GetCurrentDirectory(len + 1, p);
						nana::string s = p;
						delete[] p;
						return s;
					}
					return nana::string(buf);
				}
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				const char * s = ::getenv("PWD");
				if (s)
					return static_cast<nana::string>(nana::charset(std::string(s, std::strlen(s)), nana::unicode::utf8));
#endif
				return nana::string();
			}
		}//end namespace filesystem
	} //end namespace experimental
}//end namespace nana
