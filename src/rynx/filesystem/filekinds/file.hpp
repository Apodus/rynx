#pragma once

#include <vector>

namespace rynx
{
	namespace filesystem
	{
		class ifile
		{
		public:
			virtual ~ifile() {};
		};

		class iread_file : public ifile
		{
		public:
			virtual ~iread_file() {};
			virtual void seek_beg(int64_t pos) = 0;
			virtual void seek_cur(int64_t pos) = 0;
			virtual size_t read(void* dst, size_t bytes) = 0;
			virtual size_t tell() const = 0;
			virtual size_t size() const = 0;
			bool empty() const { return size() == 0; }

			size_t operator ()(void* dst, size_t bytes) { return read(dst, bytes); }
			template<typename T> void operator()(T& t) { operator()(&t, sizeof(t)); }
			template<typename T> size_t read(T& t) { return read(&t, sizeof(t)); }
			template<typename T> T read() { T t;  read(&t, sizeof(t)); return t; }

			std::vector<char> read_all() {
				size_t fileSize = size();
				seek_beg(0);
				std::vector<char> data(fileSize);
				read(data.data(), fileSize);
				return data;
			}
		};

		class iwrite_file : public ifile
		{
		public:
			enum class mode
			{
				Overwrite,
				Append
			};

			virtual ~iwrite_file() {};

			virtual void write(const void* filecontent, size_t bytes) = 0;
			template<typename T> void write(const T& t) { write(&t, sizeof(t)); }
			void operator()(const void* filecontent, size_t bytes) { write(filecontent, bytes); }
			template<typename T> void operator()(const T& t) { operator()(&t, sizeof(t)); }
		};
	}
}