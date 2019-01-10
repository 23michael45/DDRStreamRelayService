#include "FileManager.h"
#include "../Client/GlobalManager.h"

FileManager::FileManager()
{

	m_RootPath = GlobalManager::Instance()->GetConfig().GetValue("HttpRoot");

}
FileManager::~FileManager()
{

}
