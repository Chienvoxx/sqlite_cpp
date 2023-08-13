
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


#include <stdio.h>
#include <chrono>
#include <thread>

#include "SQLite.h"

namespace voxx
{
	namespace db
	{
		void Entry();
	}
}

int main()
{
	voxx::db::Entry();
}

namespace voxx
{
	namespace db
	{
		void Entry()
		{
			//sqlite3* connection = nullptr;

			//int result = sqlite3_open(":memory:", connection.Set());
			//if (SQLITE_OK != result)
			//{
			//	printf("%s\n", sqlite3_errmsg(connection.Get()));
			//	return result;
			//}

			try
			{
				Connection connection = Connection::Memory(); // an in-memory database created by default
				// <Profile>
				connection.Profile([](void*, char const* const statement, unsigned long long const time) // time in nanoseconds (billionth)
					{
						std::string duration;
						//unsigned long long const ms = time / 1000;		// convert to microseconds (millionth)
						//auto duration = "microseconds";
						unsigned long long const ms = time / 1000000;		// convert to milliseconds (thousandth)
						duration = "milliseconds";
						if (ms > 10)
						{
							// statement won't print the bound values, only the original text
							printf("Profiler (connection): (%lld) %s\n\t%s\n", ms, duration.c_str(), statement);
						}
					});
				// </Profile>

				//{
				//	Statement statement;
				//	statement.Prepare(connection, "select ?1 union all select ?2");
				//	statement.Bind(1, std::string("Hello"));
				//	statement.Bind(2, std::wstring(L"World"));
				//	auto result = statement.Step();
				//	printf("%s\n", statement.GetString(0));
				//}

				std::string hello = "Hello";
				//std::wstring world = L"World";
				//statement.BindAll(hello, world);

				//	Create, Bind, and prepare a Statement in one call.
				//	Accepts variadic arguments.
				//	Other specialized bind methods to be written as needed.
				Statement statement(connection, "select ?1 union all select ?2", hello, std::wstring(L"World"));
				printf("Inserted %lld\n", connection.RowId());

				////statement.Prepare(connection, "select ?1 union all select ?2", hello, std::wstring(L"World"));

				for (Row const& row : statement)
				{
					printf("%s: %s\n", row.GetString(0), SQLiteTypeName(row.GetType(0)));
				}

				Execute(connection, "create table Users (Name)");
				Execute(connection, "insert into Users values (?)", "Joe");
				printf("Inserted %lld\n", connection.RowId());

				Statement stmt(connection, "select * from Users where RowId = ?", connection.RowId());
				auto result = stmt.Step();

				printf("%s\n", stmt.GetString(0));

				result = stmt.Step();

				Execute(connection, "insert into Users values (?)", "Beth");
				printf("Inserted %lld\n", connection.RowId());

				Statement userStmt(connection, "select * from Users");

				for (Row const& row : Statement(connection, "select RowId, Name from Users"))
				{
					printf("%d: %s: %s\n", row.GetInt(0), row.GetString(1), SQLiteTypeName(row.GetType(1)));
				}

				Execute(connection, "drop table if exists Things");
				Execute(connection, "create table Things (Content)");

				// <Reset Binding Values>
				Statement thingsStmt(connection, "insert into Things values (?)");

				// How to do all transactions as one...
				// <Control number of transactions>
				// <Begin - Commit>
				Execute(connection, "begin");
				for (int i = 0; i != 1000000; ++i)
				{
					// <Reset Binding Values>
					thingsStmt.Reset(i);
					thingsStmt.Execute();
				}
				Execute(connection, "commit");
				// <Begin - Commit>
				// </Control number of transactions>
				
				// <Backup>
				if (0)
				{
					// https://www.sqlite.org/backup.html
					// https://www.sqlite.org/c3ref/backup_finish.html
					// https://www.sqlite.org/lang_vacuum.html
					// vacuum is a single exclusing transaction. You cannot do any other database work while it is in progress.
					// Can potentially use up a lot of disk space, depending on how large your database is to begin with.
					SaveToDisk(connection, "backupRaw.db");
					SaveToDisk(connection, "backupNeedVacuum.db");
					Execute(connection, "delete from Things where Content > 10");
					//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					Execute(connection, "vacuum");
					SaveToDisk(connection, "backupGotVacuum.db");
				}
				// </Backup>


				// Open, create, use other databases on the fly
				if (0)
				{
					Connection transactions("transactions.db");
					
					// time is in nanoseconds
					transactions.Profile([](void*, char const* const statement, unsigned long long const time)
						{
							// convert to milliseconds
							unsigned long long const ms = time / 1000000;
							if (ms > 1)
							{
								// statement won't print the bound values, only the original text
								printf("transactions Profiler (%lld) %s\n", ms, statement);
							}
						});

					Execute(transactions, "drop table if exists Things");
					Execute(transactions, "create table Things (Content)");
					Statement insertMulti(transactions, "insert into Things (Content) values (?)");
					Execute(transactions, "begin");

					for (int i = 0; i != 1000; ++i)
					{
						insertMulti.Reset(i);
						insertMulti.Execute();
					}

					Execute(transactions, "commit");

					Statement count(transactions, "select count(*) from Things");
					count.Step();
					printf("count: %d\n", count.GetInt());
				}
				// </Control number of transactions>




			}
			catch (Exception const& e)
			{
				printf("%s (%d)\n", e.Message.c_str(), e.Result);
				bool pause = true;
			}

			bool pause = true;
		}
	}
}