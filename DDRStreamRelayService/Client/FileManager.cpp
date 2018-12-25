#include "FileManager.h"
#include "GlobalManager.h"

FileManager::FileManager()
{

	m_RootPath = GlobalManager::Instance()->GetConfig().GetValue("HttpRoot");

}
FileManager::~FileManager()
{

}
