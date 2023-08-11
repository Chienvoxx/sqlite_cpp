
// --------------------------------------------------------------
// From the Pluralsight course
//	SQLite with Modern C++
//		Kenny Kerr
//		http://kennykerr.ca
//		Released	17 Feb 2015
//		Reviewed	31 Oct 2022
//	https://app.pluralsight.com/course-player?clipId=6a37d17c-3a2c-432c-a6e6-53eb06b84935
//
//	I completed the course over the few days ending 2023 0730
// --------------------------------------------------------------

// Other of his resources
// SQLite with C++
//		https://kennykerrca.wordpress.com/2014/04/15/sqlite-with-c/
// SQLite Performance and Prepared Statements
// https://visualstudiomagazine.com/articles/2014/03/01/sqlite-performance-and-prepared-statements.aspx

#pragma once

// For some reason this is not working for me. But adding,
//		sqlite3_config(SQLITE_CONFIG_URI,1)
// before opening database does.
//#ifndef SQLITE_USE_URI
//#define  SQLITE_USE_URI 1
//#endif

#include "sqlite3.h"
#include "Handle.h"
#include <string>

//	https://www.sqlite.org/capi3ref.html

// SQLite Results from a Query
// https://www.sqlite.org/c3ref/column_blob.html
//	const void*				sqlite3_column_blob		(sqlite3_stmt*, int iCol);
//	double					sqlite3_column_double	(sqlite3_stmt*, int iCol);
//	int						sqlite3_column_int		(sqlite3_stmt*, int iCol);
//	sqlite3_int64			sqlite3_column_int64	(sqlite3_stmt*, int iCol);
//	const unsigned char*	sqlite3_column_text		(sqlite3_stmt*, int iCol);
//	const void*				sqlite3_column_text16	(sqlite3_stmt*, int iCol);
//	sqlite3_value*			sqlite3_column_value	(sqlite3_stmt*, int iCol);
//	int						sqlite3_column_bytes	(sqlite3_stmt*, int iCol);
//	int						sqlite3_column_bytes16	(sqlite3_stmt*, int iCol);
//	int						sqlite3_column_type		(sqlite3_stmt*, int iCol);

//	Sqlite Result Code List
//	https://www.sqlite.org/rescode.html
//	(0)  SQLITE_OK
//	(1)  SQLITE_ERROR
//	(5)  SQLITE_BUSY
// 
//	(12) SQLITE_NOTFOUND
// 
//	(16) SQLITE_EMPTY
// 
//	(19) SQLITE_CONSTRAINT
//	
//	(21) SQLITE_MISUSE
//	
//	(100) SQLITE_ROW
//	(101) SQLITE_DONE

#ifdef _DEBUG
#define VERIFY ASSERT
#define VERIFY_(result, expression) ASSERT(result == expression)
#else
#define VERIFY(expression) (expression)
#define VERIFY_(result, expression) (expression)
#endif

namespace voxx
{
	namespace db
	{

		//	SQLite.h
		//		enum class Type
		//		static char const* SQLiteTypeName(Type const type)
		//		struct Exception
		//		class Connection
		//		class Backup
		//		template<typename T>
		//			struct Reader
		//		class Row : public Reader<Row>
		//		class Statement : public Reader<Statement>
		//		class RowIterator
		//		inline RowIterator begin(Statement const& statement) noexcept
		//		inline RowIterator end(Statement const& statement) noexcept
		//		template <typename C, typename ... Values>
		//			void Execute(Connection const& connection, C const* const text, Values && ... values)
		//		static void SaveToDisk(Connection const& source, char const* const filename)


		enum class Type
		{
			Integer = SQLITE_INTEGER,
			Float = SQLITE_FLOAT,
			Blob = SQLITE_BLOB,
			Null = SQLITE_NULL,
			Text = SQLITE_TEXT
		};


		static char const* SQLiteTypeName(Type const type)
		{
			switch (type)
			{
			case Type::Integer: return "Integer";
			case Type::Float: return "Float";
			case Type::Blob: return "Blob";
			case Type::Null: return "Null";
			case Type::Text: return "Text";
			}

			ASSERT(false);
			return "Invalid";
		}


		struct Exception
		{
			int Result = 0;
			std::string Message;

			explicit Exception(sqlite3* const connection) :
				Result(sqlite3_extended_errcode(connection)),
				Message(sqlite3_errmsg(connection))
			{}
		};


		// sqlite3*
		class Connection
		{
			struct ConnectionHandleTraits : HandleTraits<sqlite3*>
			{
				static void Close(Type value) noexcept
				{
					VERIFY_(SQLITE_OK, sqlite3_close(value));

					//VERIFY_
					// ^^^^^^^^
					//(void)((!!((0 == sqlite3_close(value)))) || (1 != _CrtDbgReportW(2, L"C:\\dev\\learn-sqlite\\SampleSqlite\\SampleSqlite\\SQLite.h", 34, 0, L"%ls", L"0 == sqlite3_close(value)")) || (__debugbreak(), 0));

					//ASSERT(SQLITE_OK, sqlite3_close(value));
					// ^^^^^^^^
					//_ASSERTE(SQLITE_OK, sqlite3_close(value));
					// ^^^^^^^^
					//(void)((!!((0                        ))) || (1 != _CrtDbgReportW(2, L"C:\\dev\\learn-sqlite\\SampleSqlite\\SampleSqlite\\SQLite.h", 35, 0, L"%ls", L"SQLITE_OK"                )) || (__debugbreak(), 0));
				}
			};

			using ConnectionHandle = Handle<ConnectionHandleTraits>;

			ConnectionHandle mHandle;

			// Original version had the sqlite3_open function passed in because it used the non-_v2 versions.
			// I swithced to use v2 because flags give more control of database creation settings
			// , but there is no v2 version of open16.
			// F = Function. Either sqlite3_open_v2, or sqlite3_open16_v2
			// https://www.sqlite.org/c3ref/open.html
			// Flag example: SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY | SQLITE_OPEN_SHAREDCACHE
			template <typename F, typename C>
			void InternalOpen(F open, C const* const filename, uint32_t flags)
			{
				if (flags == 0)
					flags == SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

				Connection temp;

				if (SQLITE_OK != open(filename, temp.mHandle.Set(), flags, NULL)) // open = sqlite3_open_v2
				{
					temp.ThrowLastError();
				}
				swap(mHandle, temp.mHandle);
			}

		public:

			Connection() noexcept = default;

			template <typename C>
			explicit Connection(C const* const filename, uint32_t flags = 0)
			{
				Open(filename, flags);
				//Open(filename);
			}

			static Connection Memory()
			{
				//return Connection(":memory:");
				
				// <testing>
				//sqlite3_config(SQLITE_CONFIG_URI, 1);
				//return Connection("file:memory:");
				//return Connection("file::memory:?cache=shared", SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY | SQLITE_OPEN_URI);
				// </testing>
				uint32_t flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY | SQLITE_OPEN_SHAREDCACHE;
				return Connection(":memory:", flags);
			}

			static Connection WideMemory()
			{
				return Connection(L":memory:", 0);
			}

			explicit operator bool() const noexcept
			{
				return static_cast<bool>(mHandle);
			}

			sqlite3* GetAbi()  const noexcept
			{
				return mHandle.Get();
			}

			__declspec(noreturn) void ThrowLastError() const
			{
				throw Exception(GetAbi());
			}


			void Open(char const* const filename, uint32_t flags = 0)
			{
				//InternalOpen(sqlite3_open, filename);
				InternalOpen(sqlite3_open_v2, filename, flags);
			}

			void Open(wchar_t const* const filename, uint32_t flags = 0)
			{
				//InternalOpen(sqlite3_open16, filename);
				Connection temp;

				if (SQLITE_OK != sqlite3_open16(filename, temp.mHandle.Set()))
				{
					temp.ThrowLastError();
				}
				swap(mHandle, temp.mHandle);
			}

			long long RowId() const noexcept
			{
				return sqlite3_last_insert_rowid(GetAbi());
			}

			// Set a profiling callback
			template < typename F>
			void Profile(F callback, void* const context = nullptr)
			{
				sqlite3_profile(GetAbi(), callback, context);
			}
		};


		// https://www.sqlite.org/backup.html
		// sqlite3_backup*
		class Backup
		{
			struct BackupHandleTraits : HandleTraits<sqlite3_backup*>
			{
				static void Close(Type value) noexcept
				{
					sqlite3_backup_finish(value);
				}
			};

			//using BackupHandle = Handle<BackupHandleTraits>;
			//BackupHandle mHandle;
			Handle<BackupHandleTraits> mHandle;	// this appears equivalent to the above two lines.
			Connection const* mDestination = nullptr;

		public:

			Backup(Connection const& destination
					, Connection const& source
					, char const* const destinationName = "main"
					, char const* const sourceName = "main")
				: mHandle(sqlite3_backup_init(destination.GetAbi(), destinationName, source.GetAbi(), sourceName))
				, mDestination(&destination)
			{
				if (!mHandle)
				{
					destination.ThrowLastError();
				}
			}

			sqlite3_backup* GetAbi() const noexcept
			{
				return mHandle.Get();
			}

			bool Step(int const pages = -1)
			{
				int const result = sqlite3_backup_step(GetAbi(), pages);

				if (result == SQLITE_OK) return true;
				if (result == SQLITE_DONE) return false;

				// error if we're at this point
				// Destination connection only recieves the error message when the backup object (mHandle) is destroyed
				mHandle.Reset();
				mDestination->ThrowLastError();
			}
		};


		template<typename T>
		struct Reader
		{
			int GetInt(int const column = 0) const noexcept
			{
				return sqlite3_column_int(static_cast<T const*>(this)->GetAbi(), column);
			}

			char const* GetString(int const column = 0) const noexcept
			{
				return reinterpret_cast<const char*>(sqlite3_column_text(
					static_cast<T const*>(this)->GetAbi(), column));
			}

			wchar_t const* GetWideString(int const column = 0) const noexcept
			{
				return reinterpret_cast<const wchar_t*>(sqlite3_column_text16(
					static_cast<T const*>(this)->GetAbi(), column));
			}

			int GetStringLength(int const column = 0) const noexcept
			{
				// bytes function returns size of a BLOB or a UTF-8 TEXT result in bytes
				return sqlite3_column_bytes(static_cast<T const*>(this)->GetAbi(), column);
			}

			int GetWideStringLength(int const column = 0) const noexcept
			{
				// bytes function returns size of UTF-16 TEXT in bytes. Tthat is number of 8 bit bytes. So div by wchar_t
				return sqlite3_column_bytes16(static_cast<T const*>(this)->GetAbi(), column) / sizeof(wchar_t);
			}

			Type GetType(int const column = 0) const noexcept
			{
				return static_cast<Type>(sqlite3_column_type(static_cast<T const*>(this)->GetAbi(), column));
			}
		};


		// sqlite3_stmt*
		class Row : public Reader<Row>
		{
			sqlite3_stmt* mStatement = nullptr;

		public:

			sqlite3_stmt* GetAbi() const noexcept
			{
				return mStatement;
			}

			Row(sqlite3_stmt* const statement) noexcept :
				mStatement(statement)
			{}
		};


		// sqlite3_stmt*
		class Statement : public Reader<Statement>
		{
			struct StatementHandleTraits : HandleTraits<sqlite3_stmt*>
			{
				static void Close(Type value) noexcept
				{
					VERIFY_(SQLITE_OK, sqlite3_finalize(value));
				}
			};

			using StatementHandle = Handle<StatementHandleTraits>;
			StatementHandle mHandle;

			template <typename F, typename C, typename ... Values>
			void InternalPrepare(Connection const& connection, F prepare, C const* const text, Values && ... values)
			{
				ASSERT(connection);

				if (SQLITE_OK != prepare(connection.GetAbi(), text, -1, mHandle.Set(), nullptr))
				{
					connection.ThrowLastError();
				}

				BindAll(std::forward<Values>(values) ...);
			}

			// this is the base case, i.e. when there is nothing more to std::forward
			void InternalBind(int) const noexcept
			{}

			template <typename First, typename ... Rest>
			void InternalBind(int const index, First&& first, Rest&& ... rest) const
			{
				Bind(index, std::forward<First>(first));
				InternalBind(index + 1, std::forward<Rest>(rest) ...);
			}

		public:

			Statement() noexcept = default;

			template <typename C, typename ... Values>
			Statement(Connection const& connection, C const* const text, Values && ... values)
			{
				Prepare(connection, text, std::forward<Values>(values) ...);
			}

			explicit operator bool() const noexcept
			{
				return static_cast<bool>(mHandle);
			}

			sqlite3_stmt* GetAbi() const noexcept
			{
				return mHandle.Get();
			}

			__declspec(noreturn) void ThrowLastError() const
			{
				throw Exception(sqlite3_db_handle(GetAbi()));
			}

			// Connection, SQL String, bind values
			template <typename ... Values>
			void Prepare(Connection const& connection, char const* const text, Values&& ... values)
			{
				InternalPrepare(connection, sqlite3_prepare_v2, text, std::forward<Values>(values) ...);
			}

			// Connection, SQL WString, bind values
			template <typename ... Values>
			void Prepare(Connection const& connection, wchar_t const* const text, Values&& ... values)
			{
				InternalPrepare(connection, sqlite3_prepare16_v2, text, std::forward<Values>(values) ...);
			}

			bool Step() const
			{
				int const result = sqlite3_step(GetAbi());

				if (result == SQLITE_ROW) return true;
				if (result == SQLITE_DONE) return false;

				ThrowLastError();
			}

			void Execute() const
			{
				VERIFY(!Step());
			}

			void Bind(int const index, int const value) const
			{
				if (SQLITE_OK != sqlite3_bind_int(GetAbi(), index, value))
				{
					ThrowLastError();
				}
			}

			// SQLITE_STATIC - SQLite uses data at the address passed in.
			void Bind(int const index, char const* const value, int const size = -1) const
			{
				if (SQLITE_OK != sqlite3_bind_text(GetAbi(), index, value, size, SQLITE_STATIC))
				{
					ThrowLastError();
				}
			}

			void Bind(int const index, wchar_t const* const value, int const size = -1) const
			{
				if (SQLITE_OK != sqlite3_bind_text16(GetAbi(), index, value, size, SQLITE_STATIC))
				{
					ThrowLastError();
				}
			}

			void Bind(int const index, std::string const& value) const
			{
				Bind(index, value.c_str(), static_cast<const int>(value.size()));
			}

			void Bind(int const index, std::wstring const& value) const
			{
				Bind(index, value.c_str(), static_cast<const int>(value.size()) * sizeof(wchar_t));
			}

			// SQLITE_TRANSIENT - SQLite makes a copy of data
			void Bind(int const index, std::string&& value) const
			{
				if (SQLITE_OK != sqlite3_bind_text(GetAbi(), index, value.c_str(), static_cast<const int>(value.size()), SQLITE_TRANSIENT))
				{
					ThrowLastError();
				}
			}

			void Bind(int const index, std::wstring&& value) const
			{
				if (SQLITE_OK != sqlite3_bind_text16(GetAbi(), index, value.c_str(), static_cast<const int>(value.size()) * sizeof(wchar_t), SQLITE_TRANSIENT))
				{
					ThrowLastError();
				}
			}

			template <typename ... Values>
			void BindAll(Values&& ... values) const
			{
				InternalBind(1, std::forward<Values>(values) ...);
			}

			// call Reset between calls when binding different values each call.
			// Use when repeating calls to same Statement
			template <typename ... Values>
			void Reset(Values && ... values) const
			{
				if (SQLITE_OK != sqlite3_reset(GetAbi()))
				{
					ThrowLastError();
				}

				BindAll(values ...);
			}
		};


		class RowIterator
		{
			Statement const* mStatement = nullptr;

		public:
			RowIterator() noexcept = default;

			RowIterator(Statement const& statement) noexcept
			{
				if (statement.Step())
				{
					mStatement = &statement;
				}
			}

			RowIterator& operator++() noexcept
			{
				if (!mStatement->Step())
				{
					mStatement = nullptr;
				}

				return *this;
			}

			bool operator ==(RowIterator const& other) const noexcept
			{
				return mStatement == other.mStatement;
			}

			bool operator !=(RowIterator const& other) const noexcept
			{
				return mStatement != other.mStatement;
			}

			Row operator *() const noexcept
			{
				return Row(mStatement->GetAbi());
			}
		};


		inline RowIterator begin(Statement const& statement) noexcept
		{
			return RowIterator(statement);
		}


		inline RowIterator end(Statement const& statement) noexcept
		{
			return RowIterator();
		}


		template <typename C, typename ... Values>
		void Execute(Connection const& connection, C const* const text, Values && ... values)
		{
			Statement(connection, text, std::forward<Values>(values) ...).Execute();
		}


		static void SaveToDisk(Connection const& source, char const* const filename)
		{
			Connection destination(filename);
			Backup backup(destination, source);
			backup.Step();
		}
	}
}