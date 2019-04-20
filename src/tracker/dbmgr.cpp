//#include "dbmgr.h"
//#include "setting.h"
//
//using namespace cl;
//dbmgr::dbmgr()
//{
//}
//
//dbmgr::~dbmgr()
//{
//}
//int dbmgr::init()
//{
//	if (0 != m_db.connect(settingSngl::instance()->get_dbaddr().c_str()))
//		return -1;
//	if (0 != db_load())
//		return -1;
//	timerSngl::instance()->register_timer(static_cast<timerHandler*>(this), 1, 30000);
//	return 0;
//}
//void dbmgr::fini()
//{
//	m_db.disconnect();
//	timerSngl::instance()->unregister_all(static_cast<timerHandler*>(this));
//}
//void dbmgr::on_timer(int e)
//{
//	m_db.ping();
//}
//
//int dbmgr::db_load()
//{
//	return 0;
//}
//
