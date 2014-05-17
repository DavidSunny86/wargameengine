#include "3dModel.h"
#include <map>
#include <set>

class CModelManager
{
public:
	CModelManager() {}
	void DrawModel(std::string const& path, const std::set<std::string> * hideMeshes = NULL, bool vertexOnly = false);
	std::shared_ptr<IBounding> GetBoundingBox(std::string const& path);
	void LoadIfNotExist(std::string const& path);
protected:
	CModelManager(CModelManager const& other) {}
private:
	std::map<std::string, std::shared_ptr<C3DModel>> m_models;
};