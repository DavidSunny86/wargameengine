#include "GameController.h"
#include "../view/GameView.h"
#include "../model/GameModel.h"
#include "../model/ObjectGroup.h"
#include "LUARegisterFunctions.h"
#include "../Module.h"
#include "../LogWriter.h"

std::shared_ptr<CGameController> CGameController::m_instanse = NULL;

std::weak_ptr<CGameController> CGameController::GetInstance()
{
	if (!m_instanse)
	{
		m_instanse.reset(new CGameController());
		m_instanse->Init();
	}
	return std::weak_ptr<CGameController>(m_instanse);
}

CGameController::CGameController()
{
}

void CGameController::Init()
{
	m_commandHandler.reset(new CCommandHandler());
	m_network.reset(new CNetwork());
	m_lua.reset(new CLUAScript());
	RegisterFunctions(*m_lua.get());
	RegisterUI(*m_lua.get());
	RegisterObject(*m_lua.get());
	m_lua->RunScript(sModule::script);
}

void CGameController::FreeInstance()
{
	m_instanse.reset();
}

CGameController::~CGameController(void)
{
}

void CGameController::Update()
{
	m_network->Update();
	if (m_updateCallback) m_updateCallback();
	if (m_singleCallback)
	{
		m_singleCallback();
		m_singleCallback = std::function<void()>();
	}
	CGameModel::GetInstance().lock()->Update();
}

void CGameController::SelectObjectGroup(double beginX, double beginY, double endX, double endY)
{
	double minX = (beginX < endX) ? beginX : endX;
	double maxX = (beginX > endX) ? beginX : endX;
	double minY = (beginY < endY) ? beginY : endY;
	double maxY = (beginY > endY) ? beginY : endY;
	CObjectGroup* group = new CObjectGroup();
	CGameModel * model = CGameModel::GetInstance().lock().get();
	for (unsigned long i = 0; i < model->GetObjectCount(); ++i)
	{
		std::shared_ptr<IObject> object = model->Get3DObject(i);
		if (object->GetX() > minX && object->GetX() < maxX && object->GetY() > minY && object->GetY() < maxY && object->IsSelectable())
		{
			group->AddChildren(object);
		}
	}
	switch (group->GetCount())
	{
	case 0:
	{
		model->SelectObject(NULL);
		delete group;
	}break;
	case 1:
	{
		model->SelectObject(group->GetChild(0));
		delete group;
	}break;
	default:
	{
		model->SelectObject(std::shared_ptr<IObject>(group));
	}break;
	}
	if (m_selectionCallback) m_selectionCallback();
}

std::shared_ptr<IObject> CGameController::GetNearestObject(double * start, double * end)
{
	std::shared_ptr<IObject> selectedObject = NULL;
	double minDistance = 10000000.0;
	CGameModel * model = CGameModel::GetInstance().lock().get();
	for (unsigned long i = 0; i < model->GetObjectCount(); ++i)
	{
		std::shared_ptr<IObject> object = model->Get3DObject(i);
		if (!object) continue;
		std::shared_ptr<IBounding> bounding = CGameModel::GetInstance().lock()->GetBoundingBox(object->GetPathToModel());
		if (!bounding) continue;
		if (bounding->IsIntersectsRay(start, end, object->GetX(), object->GetY(), object->GetZ(), object->GetRotation(), m_selectedObjectCapturePoint))
		{
			double distance = sqrt(object->GetX() * object->GetX() + object->GetY() * object->GetY() + object->GetZ() * object->GetZ());
			if (distance < minDistance)
			{
				selectedObject = object;
				minDistance = distance;
				m_selectedObjectCapturePoint.x -= selectedObject->GetX();
				m_selectedObjectCapturePoint.y -= selectedObject->GetY();
				m_selectedObjectCapturePoint.z -= selectedObject->GetZ();
			}
		}
	}
	return selectedObject;
}

void CGameController::SelectObject(double * begin, double * end, bool add, bool noCallback)
{
	std::shared_ptr<IObject> selectedObject = GetNearestObject(begin, end);
	if (selectedObject && !selectedObject->IsSelectable())
	{
		return;
	}
	std::shared_ptr<IObject> object = CGameModel::GetInstance().lock()->GetSelectedObject();
	if (CGameModel::IsGroup(object.get()))
	{
		CObjectGroup * group = (CObjectGroup *)object.get();
		if (add)
		{
			if (group->ContainsChildren(selectedObject))
			{
				group->RemoveChildren(selectedObject);
				if (group->GetCount() == 1)//Destroy group
				{
					CGameModel::GetInstance().lock()->SelectObject(group->GetChild(0));
				}
			}
			else
			{
				group->AddChildren(selectedObject);
			}
		}
		else
		{
			if (!group->ContainsChildren(selectedObject))
			{
				CGameModel::GetInstance().lock()->SelectObject(selectedObject);
			}
			else
			{
				group->SetCurrent(selectedObject);
			}
		}
	}
	else
	{
		if (add && object != NULL)
		{
			CObjectGroup * group = new CObjectGroup();
			group->AddChildren(object);
			group->AddChildren(selectedObject);
			CGameModel::GetInstance().lock()->SelectObject(std::shared_ptr<IObject>(group));
		}
		else
		{
			CGameModel::GetInstance().lock()->SelectObject(selectedObject);
		}
	}
	if (m_selectionCallback && !noCallback) m_selectionCallback();
}

const CVector3d * CGameController::GetCapturePoint() const
{
	return &m_selectedObjectCapturePoint;
}

bool CGameController::IsObjectInteresectSomeObjects(std::shared_ptr<IObject> current)
{
	CGameModel * model = CGameModel::GetInstance().lock().get();
	std::shared_ptr<IBounding> curBox = CGameModel::GetInstance().lock()->GetBoundingBox(current->GetPathToModel());
	if (!curBox) return false;
	CVector3d curPos(current->GetCoords());
	double curAngle = current->GetRotation();
	for (size_t i = 0; i < model->GetObjectCount(); ++i)
	{
		std::shared_ptr<IObject> object = model->Get3DObject(i);
		if (!object) continue;
		std::shared_ptr<IBounding> bounding = CGameModel::GetInstance().lock()->GetBoundingBox(object->GetPathToModel());
		if (!bounding) continue;
		CVector3d pos(object->GetCoords());
		double angle = object->GetRotation();
		if (current != object && IsInteresect(curBox.get(), curPos, curAngle, bounding.get(), pos, angle))
		{
			return true;
		}
	}
	return false;
}

bool TestRay(double *origin, double *dir, IObject * shooter, IObject* target)
{
	CGameModel* model = CGameModel::GetInstance().lock().get();
	CVector3d coords;
	for (size_t i = 0; i < model->GetObjectCount(); ++i)
	{
		IObject * current = model->Get3DObject(i).get();
		if (current == shooter || current == target) continue;
		IBounding * box = model->GetBoundingBox(current->GetPathToModel()).get();
		if (!box) continue;
		if (box->IsIntersectsRay(origin, dir, current->GetX(), current->GetY(), current->GetZ(), current->GetRotation(), coords))
		{
			return false;
		}
	}
	return true;
}

int BBoxlos(double origin[3], IBounding * target, IObject * shooter, IObject * targetObject)
{
	int result = 0;
	int total = 0;
	CBoundingBox * tarBox = dynamic_cast<CBoundingBox *>(target);
	if (!tarBox)
	{
		CBoundingCompound * compound = (CBoundingCompound*)target;
		for (size_t i = 0; i < compound->GetChildCount(); ++i)
		{
			result += BBoxlos(origin, compound->GetChild(i), shooter, targetObject);
		}
		result /= compound->GetChildCount();
		total = 100;
	}
	else
	{
		double dir[3];
		for (dir[0] = tarBox->GetMin()[0] + targetObject->GetX(); dir[0] < tarBox->GetMax()[0] + targetObject->GetX(); dir[0] += (tarBox->GetMax()[0] - tarBox->GetMin()[0]) / 10.0 + 0.0001)
		{
			for (dir[1] = tarBox->GetMin()[1] + targetObject->GetY(); dir[1] < tarBox->GetMax()[1] + targetObject->GetY(); dir[1] += (tarBox->GetMax()[1] - tarBox->GetMin()[1]) / 10.0 + 0.0001)
			{
				for (dir[2] = tarBox->GetMin()[2] + targetObject->GetZ(); dir[2] < tarBox->GetMax()[2] + targetObject->GetZ(); dir[2] += (tarBox->GetMax()[2] - tarBox->GetMin()[2]) / 10.0 + 0.0001)
				{
					total++;
					if (TestRay(origin, dir, shooter, targetObject))
						result++;
				}
			}
		}
	}
	return result * 100 / total;
}

int CGameController::GetLineOfSight(IObject * shooter, IObject * target)
{
	if (!shooter || !target) return -1;
	IBounding * targetBound = CGameModel::GetInstance().lock()->GetBoundingBox(target->GetPathToModel()).get();
	double center[3] = { shooter->GetX(), shooter->GetY(), shooter->GetZ() + 2.0 };
	return BBoxlos(center, targetBound, shooter, target);
}

void CGameController::SetSelectionCallback(std::function<void()> const& onSelect)
{
	m_selectionCallback = onSelect;
}

std::vector<char> PackProperties(std::map<std::string, std::string> const&properties)
{
	std::vector<char> result;
	result.resize(4);
	*((unsigned int*)&result[0]) = properties.size();
	for (auto i = properties.begin(); i != properties.end(); ++i)
	{
		unsigned int begin = result.size();
		result.resize(begin + 10 + i->first.size() + i->second.size());
		*((unsigned int*)&result[begin]) = i->first.size() + 1;
		memcpy(&result[begin + 4], i->first.c_str(), i->first.size() + 1);
		begin += i->first.size() + 5;
		*((unsigned int*)&result[begin]) = i->second.size() + 1;
		memcpy(&result[begin + 4], i->second.c_str(), i->second.size() + 1);
	}
	return result;
}

std::vector<char> CGameController::GetState(bool hasAdresses) const
{
	CGameModel const& model = *CGameModel::GetInstance().lock();
	std::vector<char> result;
	result.resize(9);
	result[0] = 1;
	size_t count = model.GetObjectCount();
	*((unsigned int*)&result[5]) = count;
	for (size_t i = 0; i < count; ++i)
	{
		const IObject * object = model.Get3DObject(i).get();
		std::vector<char> current;
		std::string path = object->GetPathToModel();
		current.resize(36 + path.size() + 1, 0);
		*((double*)&current[0]) = object->GetX();
		*((double*)&current[8]) = object->GetY();
		*((double*)&current[16]) = object->GetZ();
		*((double*)&current[24]) = object->GetRotation();
		*((unsigned int*)&current[32]) = path.size() + 1;
		memcpy(&current[36], path.c_str(), path.size() + 1);
		if (hasAdresses)
		{
			std::vector<char> address;
			address.resize(4);
			*((unsigned int*)&address[0]) = (uintptr_t)object;
			current.insert(current.end(), address.begin(), address.end());
		}
		std::vector<char> properties = PackProperties(object->GetAllProperties());
		current.insert(current.end(), properties.begin(), properties.end());
		result.insert(result.end(), current.begin(), current.end());
	}
	std::vector<char> globalProperties = PackProperties(model.GetAllProperties());
	result.insert(result.end(), globalProperties.begin(), globalProperties.end());
	*((unsigned int*)&result[1]) = result.size();
	return result;
}

void CGameController::SetState(char* data, bool hasAdresses)
{
	unsigned int count = *(unsigned int*)&data[0];
	unsigned int current = 4;
	CGameModel * model = CGameModel::GetInstance().lock().get();
	model->Clear();
	for (size_t i = 0; i < count; ++i)
	{
		double x = *((double*)&data[current]);
		double y = *((double*)&data[current + 8]);
		double z = *((double*)&data[current + 16]);
		double rotation = *((double*)&data[current + 24]);
		unsigned int pathSize = *((unsigned int*)&data[current + 32]);
		char * path = new char[pathSize];
		memcpy(path, &data[current + 36], pathSize);
		std::shared_ptr<IObject> object = std::shared_ptr<IObject>(new CObject(path, x, y, z, rotation));
		model->AddObject(object);
		delete[] path;
		current += 36 + pathSize;
		if (hasAdresses)
		{
			unsigned int address = *((unsigned int*)&data[current]);
			current += 4;
			m_network->AddAddress(object, address);
		}
		unsigned int propertiesCount = *((unsigned int*)&data[current]);
		current += 4;
		for (unsigned int j = 0; j < propertiesCount; ++j)
		{
			unsigned int firstSize = *((unsigned int*)&data[current]);
			char * first = new char[firstSize];
			memcpy(first, &data[current + 4], firstSize);
			current += firstSize + 4;
			unsigned int secondSize = *((unsigned int*)&data[current]);
			char * second = new char[secondSize];
			memcpy(second, &data[current + 4], secondSize);
			current += secondSize + 4;
			object->SetProperty(first, second);
			delete[] first;
			delete[] second;
		}
	}
	unsigned int globalPropertiesCount = *((unsigned int*)&data[current]);
	current += 4;
	for (unsigned int i = 0; i < globalPropertiesCount; ++i)
	{
		unsigned int firstSize = *((unsigned int*)&data[current]);
		char * first = new char[firstSize];
		memcpy(first, &data[current + 4], firstSize);
		current += firstSize + 4;
		unsigned int secondSize = *((unsigned int*)&data[current]);
		char * second = new char[secondSize];
		memcpy(second, &data[current + 4], secondSize);
		current += secondSize + 4;
		model->SetProperty(first, second);
		delete[] first;
		delete[] second;
	}
}

void CGameController::Save(std::string const& filename)
{
	FILE* file = fopen(filename.c_str(), "wb");
	std::vector<char> state = CGameController::GetInstance().lock()->GetState();
	fwrite(&state[0], 1, state.size(), file);
	fclose(file);
}

void CGameController::Load(std::string const& filename)
{
	FILE* file = fopen(filename.c_str(), "rb");
	if (!file)
	{
		LogWriter::WriteLine("LoadState. Cannot find file " + filename);
		return;
	}
	fseek(file, 0L, SEEK_END);
	unsigned int size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	std::unique_ptr<char> data(new char[size]);
	fread(data.get(), 1, size, file);
	fclose(file);
	SetState(data.get() + 5);
	m_network->CallStateRecievedCallback();
}

void CGameController::TryMoveSelectedObject(std::shared_ptr<IObject> object, double x, double y, double z)
{
	if (!object)
	{
		return;
	}
	CVector3d old(object->GetCoords());
	if (CGameModel::GetInstance().lock()->GetLandscape().isCoordsOnTable(x, y))
	{
		object->SetCoords(x - m_selectedObjectCapturePoint.x, y - m_selectedObjectCapturePoint.y, z);
	}
	if (CGameController::GetInstance().lock()->IsObjectInteresectSomeObjects(object))
	{
		object->SetCoords(old);
	}
}

void CGameController::SetUpdateCallback(std::function<void()> const& onUpdate)
{
	m_updateCallback = onUpdate;
}

void CGameController::SetSingleCallback(std::function<void()> const& onSingleUpdate)
{
	m_singleCallback = onSingleUpdate;
}

bool operator< (CGameController::sKeyBind const& one, CGameController::sKeyBind const& two)
{
	return one.key < two.key;
}

void CGameController::BindKey(unsigned char key, bool shift, bool ctrl, bool alt, std::function<void()> const& func)
{
	sKeyBind keybind(key, shift, ctrl, alt);
	if (func)
	{
		m_keyBindings[keybind] = func;
	}
	else
	{
		if (m_keyBindings.find(keybind) != m_keyBindings.end())
		{
			m_keyBindings.erase(keybind);
		}
	}
}

bool CGameController::OnKeyPress(unsigned char key, bool shift, bool ctrl, bool alt)
{
	sKeyBind keybind(key, shift, ctrl, alt);
	if (m_keyBindings.find(keybind) != m_keyBindings.end())
	{
		m_keyBindings[keybind]();
		return true;
	}
	return false;
}

void CGameController::MoveObject(std::shared_ptr<IObject> obj, double deltaX, double deltaY)
{
	m_commandHandler->AddNewMoveObject(obj, deltaX, deltaY);
}

void CGameController::RotateObject(std::shared_ptr<IObject> obj, double deltaRot)
{
	m_commandHandler->AddNewRotateObject(obj, deltaRot);
}

std::shared_ptr<IObject> CGameController::CreateObject(std::string const& model, double x, double y, double rotation)
{
	std::shared_ptr<IObject> object = std::make_shared<CObject>(model, x, y, 0.0, rotation);
	m_commandHandler->AddNewCreateObject(object);
	return object;
}

void CGameController::DeleteObject(std::shared_ptr<IObject> obj)
{
	m_commandHandler->AddNewDeleteObject(obj);
}

void CGameController::SetObjectProperty(std::shared_ptr<IObject> obj, std::string const& key, std::string const& value)
{
	m_commandHandler->AddNewChangeProperty(obj, key, value);
}

CCommandHandler & CGameController::GetCommandHandler()
{
	return *m_commandHandler;
}

CNetwork& CGameController::GetNetwork()
{
	return *m_network;
}
