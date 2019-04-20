#include "test_leveldb.h"
#include <iostream>

#ifdef _MSC_VER
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

int test_leveldb()
{
	using namespace std;

	//open db
	leveldb::DB* db = NULL;
	leveldb::Options options;
	options.create_if_missing = true;
	leveldb::Status s;
	leveldb::WriteOptions wop;
	string val;
	leveldb::Slice sl;

	s = leveldb::DB::Open(options, "./testldb", &db);
	if (!s.ok())
	{
		cerr << s.ToString() << endl;
		return 0;
	}

	//put / update
	s = db->Put(leveldb::WriteOptions(), "key", "val");

	//get
	s = db->Get(leveldb::ReadOptions(), "key", &val);

	//delete
	s = db->Delete(leveldb::WriteOptions(), "key");

	//sync write: 写入硬盘后才返回
	wop.sync = true;
	s = db->Put(wop, "key", "val");

	//原子操作：
	leveldb::WriteBatch batch;
	batch.Delete("key");
	batch.Put("key", "val1");
	batch.Put("key2", "val2");
	s = db->Write(wop, &batch);

	//历遍
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		cout << it->key().ToString() << ": " << it->value().ToString() << endl;
	}
	delete it;

	//close db
	delete db;
	return 0;
}
#endif

